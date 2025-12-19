// Wii IR
// A homebrew implementation of what the TV Friend Channel did, but with more support.

// LSB First - X100001Y ; X is the first bit, Y is the last bit.
// MSB First - X100001Y ; Y is the first bit, X is the last bit.

// FIXME:   Fix and verify Samsung32 protocol.
// TODO:    Implement and verify: ITT, JVC, NRC17, RC6, RCMM, RECS80, SHARP, and XSAT.
// TODO:    Adjust timing and verify for RC5, SIRC(12,15,20).
// TODO:    Implement a way to calculate correct timings for RAW command types.
// TODO:    Adjust database and enumerators to allow for new protocols.

/*
    NEC Protocol Notes:
        16 bit address version, 8 bit command. (Only command checksum included).
        8  bit adddress version, 8 bit command (Address and command checksum included).
        Pulse distance modulation.
        Carrier frequency of 38KHz.
        Bit time of 1.125ms or 2.25ms.
        LSB First.

    SIRC Protocol Notes:
        12-bit version, 7 command bits, 5 address bits.
        15-bit version, 7 command bits, 8 address bits.
        20-bit version, 7 command bits, 5 address bits, 8 extended bits.
        Pulse width modulation.
        Carrier frequency of 40kHz.
        Bit time of 1.2ms or 0.6ms.
        LSB First.
*/


/*
 * Copyright 2025 Dakota Thorpe and Larsen Vallecillo
 *
 * This file is part of the Wii IR project.
 *
 * Permission is hereby granted to view the source code of this project for informational purposes only.
 *
 * The following rights are explicitly prohibited for all individuals and entities except Dakota Thorpe 
 * and Larsen Vallecillo:
 * - Copying
 * - Modifying
 * - Distributing
 * - Sharing
 * - Using
 *
 * No part of this project may be reproduced, distributed, or transmitted in any form or by any means, 
 * including but not limited to copying, modification, or incorporation into other projects, without 
 * prior written permission from Dakota Thorpe and Larsen Vallecillo.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
    REFERENCES:
    https://www.sbprojects.net/knowledge/ir/
    https://www.sbprojects.net/knowledge/ir/index.php
    https://www.sbprojects.net/knowledge/ir/itt.php
    https://www.sbprojects.net/knowledge/ir/jvc.php
    https://www.sbprojects.net/knowledge/ir/nec.php
    https://www.sbprojects.net/knowledge/ir/nrc17.php
    https://www.sbprojects.net/knowledge/ir/others.php
    https://www.sbprojects.net/knowledge/ir/rc5.php
    https://www.sbprojects.net/knowledge/ir/rc6.php
    https://www.sbprojects.net/knowledge/ir/rca.php
    https://www.sbprojects.net/knowledge/ir/rcmm.php
    https://www.sbprojects.net/knowledge/ir/recs80.php
    https://www.sbprojects.net/knowledge/ir/sharp.php
    https://www.sbprojects.net/knowledge/ir/sirc.php
    https://www.sbprojects.net/knowledge/ir/universal.php
    https://www.sbprojects.net/knowledge/ir/xsat.php
    https://tasmota.github.io/docs/IRSend-RAW-Encoding/
*/

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "WiiIR/IR.hpp"
#include "stb/stb_image_resize2.h"
#include <stdio.h>

#ifdef NINTENDOWII
#include <sys/wait.h>
#endif
#include <unistd.h>

// Text Files
#include "CREDITS_txt.h"
#include "LICENSE_txt.h"

#include "cJSON.h"
#include "tinyxml2.h"
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace tinyxml2;
namespace fs = std::filesystem;

#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// Trim helper
static inline std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Convert "0000" → integer
static inline u16 hexToUInt16(const std::string &hex)
{
    return (u16)strtol(hex.c_str(), nullptr, 16);
}

// --------------------------------------------------------------------------------------------
// SEND IR MAIN FUNCTION
// --------------------------------------------------------------------------------------------
void SendIR(const std::string &dataString)
{
    std::string data = trim(dataString);

    if (data.empty()) {
        printf("[SendIR] Empty data string.\n");
        return;
    }

    // Uppercase a copy for type detection
    std::string upper = data;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // ======================================================
    // =================== NEC ==============================
    // ======================================================
    if (upper.rfind("NEC:", 0) == 0)
    {
        printf("[SendIR] NEC packet: %s\n", data.c_str());

        std::string body = data.substr(4);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid NEC format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        uint8_t adr = (uint8_t)strtol(adrStr.c_str(), nullptr, 10);
        uint8_t cmd = (uint8_t)strtol(cmdStr.c_str(), nullptr, 10);

        printf("[SendIR] Calling IR_SendNEC(%u, %u)\n", adr, cmd);
        IR_SendNEC(adr, cmd);
        return;
    }

    // ======================================================
    // =================== Samsung32 ========================
    // ======================================================
    if (upper.rfind("SAMSUNG32:", 0) == 0)
    {
        printf("[SendIR] Samsung32 packet: %s\n", data.c_str());

        std::string body = data.substr(10);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid Samsung32 format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        uint8_t adr = (uint8_t)strtol(adrStr.c_str(), nullptr, 10);
        uint8_t cmd = (uint8_t)strtol(cmdStr.c_str(), nullptr, 10);

        printf("[SendIR] Calling IR_SendSamsung32(%u, %u)\n", adr, cmd);
        IR_SendSamsung32(adr, cmd);
        return;
    }

    if (upper.rfind("SIRC:", 0) == 0)
    {
        printf("[SendIR] SIRC12 packet: %s\n", data.c_str());

        std::string body = data.substr(5);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid SIRC format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        uint8_t adr = (uint8_t)strtol(adrStr.c_str(), nullptr, 10);
        uint16_t cmd = (uint16_t)strtol(cmdStr.c_str(), nullptr, 10);

        printf("[SendIR] Calling IR_SendSIRC(SONY12, %u, %u)\n", adr, cmd);
        IR_SendSIRC(IR_SIRC_MODE_12, adr, cmd);
        return;
    }

    if (upper.rfind("SIRC15:", 0) == 0)
    {
        printf("[SendIR] SIRC15 packet: %s\n", data.c_str());

        std::string body = data.substr(7);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid SIRC format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        uint8_t adr = (uint8_t)strtol(adrStr.c_str(), nullptr, 10);
        uint16_t cmd = (uint16_t)strtol(cmdStr.c_str(), nullptr, 10);

        printf("[SendIR] Calling IR_SendSIRC(SONY15, %u, %u)\n", adr, cmd);
        IR_SendSIRC(IR_SIRC_MODE_15, adr, cmd);
        return;
    }

    if (upper.rfind("SIRC20:", 0) == 0)
    {
        printf("[SendIR] SIRC20 packet: %s\n", data.c_str());

        std::string body = data.substr(7);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid SIRC format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        uint8_t adr = (uint8_t)strtol(adrStr.c_str(), nullptr, 10);
        uint16_t cmd = (uint16_t)strtol(cmdStr.c_str(), nullptr, 10);

        printf("[SendIR] Calling IR_SendSIRC(SONY20, %u, %u)\n", adr, cmd);
        IR_SendSIRC(IR_SIRC_MODE_12, adr, cmd);
        return;
    }

    // ======================================================
    // =================== NEC EXTENDED =====================
    // ======================================================
    if (upper.rfind("NECEXT:", 0) == 0)
    {
        printf("[SendIR] NECext packet: %s\n", data.c_str());

        std::string body = data.substr(7);
        size_t commaPos = body.find(',');
        if (commaPos == std::string::npos) {
            printf("[SendIR] Invalid NECext format.\n");
            return;
        }

        std::string adrStr = trim(body.substr(0, commaPos));
        std::string cmdStr = trim(body.substr(commaPos + 1));

        int adrFull = strtol(adrStr.c_str(), nullptr, 10);
        int cmdFull = strtol(cmdStr.c_str(), nullptr, 10);

        uint8_t adrLo = adrFull & 0xFF;
        uint8_t adrHi = (adrFull >> 8) & 0xFF;

        printf("[SendIR] Calling IR_SendNECext(%u, %u, %u, %u)\n",
               adrLo, adrHi, cmdFull, cmdFull);

        IR_SendNECext(adrLo, adrHi, cmdFull, cmdFull, true);
        return;
    }

    // ======================================================
    // =================== RAW / PRONTO =====================
    // ======================================================
    if (upper.rfind("RAW:", 0) == 0)
    {
        printf("[SendIR] RAW packet: %s\n", data.c_str());

        std::string body = data.substr(4);

        std::stringstream ss(body);
        std::string token;

        std::vector<u16> pronto;

        while (ss >> token)
        {
            // Uppercase hex
            std::string clean;
            for (char c : token)
                if (isxdigit(c)) clean += c;

            if (!clean.empty())
                pronto.push_back(hexToUInt16(clean));
        }

        if (pronto.empty()) {
            printf("[SendIR] No RAW/pronto data found.\n");
            return;
        }

        printf("[SendIR] Calling IR_SendPronto() with %d entries.\n", (int)pronto.size());
        IR_SendPronto(pronto.data(), pronto.size());
        return;
    }

    // ======================================================
    // =================== UNKNOWN TYPE =====================
    // ======================================================
    printf("[SendIR] Unknown IR format: %s\n", data.c_str());
}

// --- Helper to merge custom maps into a button entry ---
void MergeButtonEntry(ButtonEntry &btn, tinyxml2::XMLElement *customBtnNode) {
    // Override maps if custom exists
    tinyxml2::XMLElement* mapsNode = customBtnNode->FirstChildElement("Maps");
    if (mapsNode) {
        btn.maps.clear(); // replace with custom maps
        for (tinyxml2::XMLElement* map = mapsNode->FirstChildElement("Map"); map; map = map->NextSiblingElement("Map")) {
            MapEntry me;
            const char* text = map->GetText();
            if (text) me.value = text;
            btn.maps.push_back(me);
        }
    }

    // Override data if custom exists
    tinyxml2::XMLElement* dataNode = customBtnNode->FirstChildElement("Data");
    if (dataNode && dataNode->GetText()) {
        btn.data = dataNode->GetText();
    }
}

void AddCustomMap(const std::string &mfgName, const std::string &dvcName, const std::vector<std::string> &mapStringArray, const std::string &btnName, const std::string &customFile = "custom_maps.xml")
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root = nullptr;

    // Load file or create a new one
    if (fs::exists(customFile))
    {
        if (doc.LoadFile(customFile.c_str()) != XML_SUCCESS)
        {
            std::cerr << "Failed to load custom maps file. Creating a new one.\n";
            doc.Clear();
        }
        else
        {
            root = doc.FirstChildElement("CustomMapper");
        }
    }

    if (!root)
    {
        root = doc.NewElement("CustomMapper");
        doc.InsertFirstChild(root);
    }

    // -------------------------------
    // Manufacturer lookup/create
    // -------------------------------
    tinyxml2::XMLElement *mfgElem = nullptr;
    for (tinyxml2::XMLElement *m = root->FirstChildElement("Manufacturer"); m;
         m = m->NextSiblingElement("Manufacturer"))
    {
        if (const char *name = m->Attribute("name");
            name && mfgName == name)
        {
            mfgElem = m;
            break;
        }
    }

    if (!mfgElem)
    {
        mfgElem = doc.NewElement("Manufacturer");
        mfgElem->SetAttribute("name", mfgName.c_str());
        root->InsertEndChild(mfgElem);
    }

    // -------------------------------
    // Device lookup/create
    // -------------------------------
    tinyxml2::XMLElement *devElem = nullptr;
    for (tinyxml2::XMLElement *d = mfgElem->FirstChildElement("DeviceEntry"); d;
         d = d->NextSiblingElement("DeviceEntry"))
    {
        if (const char *name = d->Attribute("name");
            name && dvcName == name)
        {
            devElem = d;
            break;
        }
    }

    if (!devElem)
    {
        devElem = doc.NewElement("DeviceEntry");
        devElem->SetAttribute("name", dvcName.c_str());
        mfgElem->InsertEndChild(devElem);
    }

    // -------------------------------
    // REMOVE OLD ButtonEntry BY NAME
    // -------------------------------
    for (tinyxml2::XMLElement *b = devElem->FirstChildElement("ButtonEntry"); b; )
    {
        tinyxml2::XMLElement *next = b->NextSiblingElement("ButtonEntry");

        if (const char *name = b->Attribute("name");
            name && btnName == name)
        {
            devElem->DeleteChild(b);
        }

        b = next;
    }

    // -------------------------------
    // Create new ButtonEntry
    // -------------------------------
    tinyxml2::XMLElement *btnElem = doc.NewElement("ButtonEntry");
    btnElem->SetAttribute("name", btnName.c_str());
    devElem->InsertEndChild(btnElem);

    // Create new Maps
    tinyxml2::XMLElement *mapsElem = doc.NewElement("Maps");
    btnElem->InsertEndChild(mapsElem);

    for (const std::string &mapVal : mapStringArray)
    {
        tinyxml2::XMLElement *mapElem = doc.NewElement("Map");
        mapElem->SetText(mapVal.c_str());
        mapsElem->InsertEndChild(mapElem);
    }

    // Save file
    if (doc.SaveFile(customFile.c_str()) != XML_SUCCESS)
    {
        std::cerr << "Failed to save custom maps file!\n";
    }
    else
    {
        std::cout << "Custom maps updated for: " << btnName << "\n";
    }
}

// --- Load custom maps if file exists ---
void ApplyCustomMaps(XMLDatabase &db, const char* customFile) {
    if (!fs::exists(customFile)) return;

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(customFile) != XML_SUCCESS) {
        std::cerr << "Failed to load custom maps file: " << customFile << std::endl;
        return;
    }

    tinyxml2::XMLElement* root = doc.FirstChildElement("CustomMapper");
    if (!root) return;

    for (tinyxml2::XMLElement* m = root->FirstChildElement("Manufacturer"); m; m = m->NextSiblingElement("Manufacturer")) {
        const char* mname = m->Attribute("name");
        if (!mname) continue;

        // Find matching manufacturer
        for (auto &mf : db.manufacturers) {
            if (mf.name != mname) continue;

            for (tinyxml2::XMLElement* d = m->FirstChildElement("DeviceEntry"); d; d = d->NextSiblingElement("DeviceEntry")) {
                const char* dname = d->Attribute("name");
                if (!dname) continue;

                // Find matching device
                for (auto &dev : mf.devices) {
                    if (dev.name != dname) continue;

                    for (tinyxml2::XMLElement* b = d->FirstChildElement("ButtonEntry"); b; b = b->NextSiblingElement("ButtonEntry")) {
                        const char* bname = b->Attribute("name");
                        if (!bname) continue;

                        // Find matching button
                        for (auto &btn : dev.buttons) {
                            if (btn.name == bname) {
                                MergeButtonEntry(btn, b);
                            }
                        }
                    }
                }
            }
        }
    }
}

// --- Main XML loader ---
XMLDatabase LoadXML(const char* filename, const char* customFile) {
    XMLDatabase db;
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(filename) != XML_SUCCESS)
        throw std::runtime_error("Failed to load XML file.");

    tinyxml2::XMLElement* root = doc.FirstChildElement("Manufacturers");
    if (!root)
        throw std::runtime_error("Missing <Manufacturers> root!");

    // ---------------------------------------------------------
    // Iterate Manufacturers
    // ---------------------------------------------------------
    for (tinyxml2::XMLElement* m = root->FirstChildElement("Manufacturer"); m; m = m->NextSiblingElement("Manufacturer")) {
        Manufacturer mf;
        const char* name = m->Attribute("name");
        if (!name)
            throw std::runtime_error("Manufacturer missing 'name' attribute.");
        mf.name = name;

        // DeviceList
        tinyxml2::XMLElement* deviceList = m->FirstChildElement("DeviceList");
        if (deviceList) {
            // Iterate Devices
            for (tinyxml2::XMLElement* d = deviceList->FirstChildElement("DeviceEntry"); d; d = d->NextSiblingElement("DeviceEntry")) {
                DeviceEntry dev;
                const char* dname = d->Attribute("name");
                if (!dname)
                    throw std::runtime_error("DeviceEntry missing 'name' attribute.");
                dev.name = dname;

                // ButtonEntry list
                for (tinyxml2::XMLElement* b = d->FirstChildElement("ButtonEntry"); b; b = b->NextSiblingElement("ButtonEntry")) {
                    ButtonEntry btn;
                    const char* bname = b->Attribute("name");
                    if (!bname)
                        throw std::runtime_error("ButtonEntry missing 'name'");
                    btn.name = bname;

                    // Maps (nested inside <Maps>)
                    tinyxml2::XMLElement* mapsNode = b->FirstChildElement("Maps");
                    if (mapsNode) {
                        for (tinyxml2::XMLElement* map = mapsNode->FirstChildElement("Map"); map; map = map->NextSiblingElement("Map")) {
                            MapEntry me;
                            const char* text = map->GetText();
                            if (text)
                                me.value = text;
                            btn.maps.push_back(me);
                        }
                    }

                    // Data
                    tinyxml2::XMLElement* dataNode = b->FirstChildElement("Data");
                    if (dataNode && dataNode->GetText())
                        btn.data = dataNode->GetText();

                    dev.buttons.push_back(btn);
                }

                mf.devices.push_back(dev);
            }
        }

        db.manufacturers.push_back(mf);
    }

    // ---------------------------------------------------------
    // Apply custom maps if available
    // ---------------------------------------------------------
    if (customFile)
        ApplyCustomMaps(db, customFile);

    return db;
}

// Call this with device.buttons from XML
// TODO: Reimplement WiiIR header to have premapped button
// definitions to avoid defining static string names in this
// function.
void RunDeviceInputLoop(const DeviceEntry& device)
{
    restore_original_cout();
    printf("=== Running Device: %s ===\n", device.name.c_str());
    printf("Press ESC (Windows) or HOME (Wii) 5 times to exit.\n\n");

    int homePressCount = 0;

    while (true)
    {
#ifdef NINTENDOWII
        // ---- Wii input ----
        WPAD_ScanPads();
        uint32_t down = WPAD_ButtonsDown(0);
        bool homePressed = (down & WPAD_BUTTON_HOME);
#else
        // ---- Windows native input ----
        bool homePressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
#endif

        // ---------------- HOME/EXIT ----------------
        if (homePressed)
        {
            homePressCount++;
            printf("[INFO] HOME/ESC pressed (%d / 5)\n", homePressCount);
            if (homePressCount >= 5)
            {
                printf("Exiting device mode.\n");
                break;
            }

#ifdef NINTENDOWII
            VIDEO_WaitVSync();
#else
            Sleep(16);
#endif
            continue;
        }
        #ifdef NINTENDOWII
        else if(down && !homePressed)
        #else
        else if(!homePressed)
        #endif
        {
            homePressCount = 0;
        }

        // ---------------- Regular button mapping ----------------
        for (const auto& btn : device.buttons)
        {
            for (const auto& map : btn.maps)
            {
#ifdef NINTENDOWII
                bool pressed = false;
                // Wii Mappingsx
                // TODO: Rewrite this
                if      (map.value == "WPAD_BUTTON_UP") pressed = (down & WPAD_BUTTON_UP) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_DOWN") pressed = (down & WPAD_BUTTON_DOWN) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_LEFT") pressed = (down & WPAD_BUTTON_LEFT) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_RIGHT") pressed = (down & WPAD_BUTTON_RIGHT) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_A") pressed = (down & WPAD_BUTTON_A) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_B") pressed = (down & WPAD_BUTTON_B) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_1") pressed = (down & WPAD_BUTTON_1) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_2") pressed = (down & WPAD_BUTTON_2) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_PLUS") pressed = (down & WPAD_BUTTON_PLUS) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_MINUS") pressed = (down & WPAD_BUTTON_MINUS) ? 1 : 0;
                if      (map.value == "WPAD_BUTTON_HOME") pressed = (down & WPAD_BUTTON_HOME) ? 1 : 0;
                if      (map.value == "WPAD_NUNCHUK_C") pressed = (down & WPAD_NUNCHUK_BUTTON_C) ? 1 : 0;
                if      (map.value == "WPAD_NUNCHUK_Z") pressed = (down & WPAD_NUNCHUK_BUTTON_Z) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_A") pressed = (down & WPAD_CLASSIC_BUTTON_A) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_B") pressed = (down & WPAD_CLASSIC_BUTTON_B) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_X") pressed = (down & WPAD_CLASSIC_BUTTON_X) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_Y") pressed = (down & WPAD_CLASSIC_BUTTON_Y) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_ZL") pressed = (down & WPAD_CLASSIC_BUTTON_ZL) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_ZR") pressed = (down & WPAD_CLASSIC_BUTTON_ZR) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_FULL_L") pressed = (down & WPAD_CLASSIC_BUTTON_FULL_L) ? 1 : 0;
                if      (map.value == "WPAD_CLASSIC_BUTTON_FULL_R") pressed = (down & WPAD_CLASSIC_BUTTON_FULL_R) ? 1 : 0;
                if (pressed)
                {
                    printf("Button pressed: %s -> Sending IR: %s\n",
                           btn.name.c_str(), btn.data.c_str());
                    SendIR(btn.data);
                }
#else
                // Windows key mapping
                bool pressed = false;
                if      (map.value == "A") pressed = (GetAsyncKeyState('A') & 0x8000) != 0;
                else if (map.value == "B") pressed = (GetAsyncKeyState('B') & 0x8000) != 0;
                else if (map.value == "UP") pressed = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
                else if (map.value == "DOWN") pressed = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
                else if (map.value == "LEFT") pressed = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
                else if (map.value == "RIGHT") pressed = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
                else if (map.value == "1") pressed = (GetAsyncKeyState('1') & 0x8000) != 0;
                else if (map.value == "2") pressed = (GetAsyncKeyState('2') & 0x8000) != 0;
                else if (map.value == "+") pressed = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) != 0;
                else if (map.value == "-") pressed = (GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) != 0;
                else if (map.value == "ENTER") pressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

                if (pressed)
                {
                    printf("Button pressed: %s -> Sending IR: %s\n",
                           btn.name.c_str(), btn.data.c_str());
                    SendIR(btn.data);
                }
#endif
            }
        }

#ifdef NINTENDOWII
        VIDEO_WaitVSync();
#else
        Sleep(16); // ~60 Hz loop
#endif
    }

    // Exit when done
    exit(0);
}

// Render built in text file content
void RenderBuiltInDocumentAsChild(const unsigned char content[], size_t content_size) {
    ImGui::BeginChild("LicenseTextRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(
        reinterpret_cast<const char*>(content),
        reinterpret_cast<const char*>(content + content_size)
    );
    ImGui::EndChild();
}

// Credits Window
void ShowCreditsWindow(bool* p_open) {
    ImGui::Begin("Software Credits", p_open, ImGuiWindowFlags_None);
    RenderBuiltInDocumentAsChild(CREDITS_txt, CREDITS_txt_size);
    ImGui::End();
}

// Open Source Licenses
void ShowOSLWindow(bool* p_open) {
    ImGui::Begin("Open Source Licenses", p_open, ImGuiWindowFlags_None);
    RenderBuiltInDocumentAsChild(LICENSE_txt, LICENSE_txt_size);
    ImGui::End();
}

void ShowBuildInfoWindow(bool* p_open) {
    ImGui::Begin("Build Information", p_open, ImGuiWindowFlags_None);
    ImGui::Text("Build Host: %s", BUILD_HOST);
    ImGui::Text("Build Target: %s", BUILD_TARG);
    ImGui::Text("Build Date: %s", __DATE__);
    ImGui::Text("Build Time: %s", __TIME__);
    ImGui::Text("ImGUI Version: v%s", IMGUI_VERSION);
    ImGui::Text("TinyXML2 Version: v%d.%d.%d", TINYXML2_MAJOR_VERSION, TINYXML2_MINOR_VERSION, TINYXML2_PATCH_VERSION);
    ImGui::Text("libcJSON Version: v%d.%d.%d", CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);
    ImGui::End();
}

void ShowDebuggerWindow(bool* p_open) {
    ImGui::Begin("WiiIR Debugger", p_open, ImGuiWindowFlags_None);

    // Show metrics
    static bool showMet = false;

    // Checkboxing
    ImGui::Checkbox("Show Metrics", &showMet);
    ImGui::TextLinkOpenURL("Go to the OldNet", "http://theoldnet.com/");
    
    // Metrics
    if(showMet) ImGui::ShowMetricsWindow(&showMet);
    ImGui::End();
}

void DrawXMLBrowser(XMLDatabase& db, ImGuiWindowFlags &window_flags)
{
    static int selectedManufacturer = -1;
    static int selectedDevice = -1;
    static int selectedButton = -1;

    // Modals
    static bool showOSLModal = false;
    static bool showBuildInfoModal = false;
    static bool showCreditsModal = false;
    static bool showDebugModal = false;
    static bool showMSPaint = false;

    // Main window
    ImGui::Begin("InfraRed Browser", nullptr, ImGuiWindowFlags_MenuBar); //, nullptr, window_flags);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                exit(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Open Source Licenses", nullptr, &showOSLModal, true);
            ImGui::MenuItem("Build Information", nullptr, &showBuildInfoModal, true);
            ImGui::MenuItem("Credits", nullptr, &showCreditsModal, true);
            ImGui::MenuItem("MSPaint", nullptr, &showMSPaint, true);
            ImGui::MenuItem("Debugger", nullptr, &showDebugModal, true);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // About Modal
    if (showCreditsModal) {
        ShowCreditsWindow(&showCreditsModal);
    } if (showBuildInfoModal) {
        ShowBuildInfoWindow(&showBuildInfoModal);
    } if (showOSLModal) {
        ShowOSLWindow(&showOSLModal);
    } if (showDebugModal) {
        ShowDebuggerWindow(&showDebugModal);
    }

    // MSPaint
    if(showMSPaint) {
        DrawMSPaintEasterEgg(&showMSPaint);
    }

    // ----- LAYOUT: 3 horizontal panels -----
    //float panelHeight = ImGui::GetContentRegionAvail().y;

    // LEFT PANEL: Manufacturers
    ImGui::BeginChild("mfg_panel", ImVec2(200, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Text("Manufacturers");
    ImGui::Separator();

    static char mfgSearch[128] = "";

    // Wii Text Hint
    #ifdef NINTENDOWII
    ImGui::InputTextWithHint("##mfg_search", "Search (USB KB)", mfgSearch, IM_ARRAYSIZE(mfgSearch));

    // PC Text Hint
    #else
    ImGui::InputTextWithHint("##mfg_search", "Search Box", mfgSearch, IM_ARRAYSIZE(mfgSearch));
    #endif

    ImGui::Separator();

    std::string needle = mfgSearch;
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

    for (int i = 0; i < db.manufacturers.size(); i++)
    {
        ImGui::PushID(i);
        const std::string& name = db.manufacturers[i].name;
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (!needle.empty() && lower.find(needle) == std::string::npos) { ImGui::PopID(); continue; }

        if (ImGui::Selectable(name.c_str(), selectedManufacturer == i))
        {
            selectedManufacturer = i;
            selectedDevice = -1;
            selectedButton = -1;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // MIDDLE PANEL: Devices
    ImGui::BeginChild("device_panel", ImVec2(250, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Text("Devices");
    ImGui::Separator();

    if (selectedManufacturer >= 0)
    {
        auto& mf = db.manufacturers[selectedManufacturer];
        for (int d = 0; d < mf.devices.size(); d++)
        {
            if (ImGui::Selectable(mf.devices[d].name.c_str(), selectedDevice == d))
            {
                selectedDevice = d;
                selectedButton = -1;
            }
        }
    }
    else
    {
        ImGui::TextDisabled("Select a manufacturer first.");
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // RIGHT PANEL: Buttons
    ImGui::BeginChild("button_panel", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Text("Buttons");
    ImGui::Separator();

    static bool showEditMappingsModal = false;
    static Manufacturer* editingMfg = nullptr;
    static DeviceEntry* editingDevice = nullptr;
    static ButtonEntry* editingButton = nullptr;
    static std::unordered_map<std::string, bool> selectedButtons;

    if (selectedManufacturer >= 0 && selectedDevice >= 0)
    {
        auto& dev = db.manufacturers[selectedManufacturer].devices[selectedDevice];

        if (dev.buttons.empty())
            ImGui::TextDisabled("No ButtonEntry found.");
        else
        {
            if (ImGui::Button("Run Device"))
            {
                ShutdownUI();
                Init();
                RunDeviceInputLoop(dev);
            }

            for (int b = 0; b < dev.buttons.size(); b++)
            {
                if (ImGui::Selectable(dev.buttons[b].name.c_str(), selectedButton == b))
                    selectedButton = b;
            }
        }

        ImGui::Separator();

        if (selectedButton >= 0 && ImGui::Button("Edit Mappings"))
        {
            showEditMappingsModal = true;
            editingMfg = &db.manufacturers[selectedManufacturer];
            editingDevice = &dev;
            editingButton = &dev.buttons[selectedButton];

            selectedButtons.clear();
            for (auto& btnName : WiiButtons) selectedButtons[btnName] = false;
            for (auto& map : editingButton->maps)
                if (selectedButtons.find(map.value) != selectedButtons.end())
                    selectedButtons[map.value] = true;

            ImGui::OpenPopup("EditDeviceMappings");
        }

        if (showEditMappingsModal && ImGui::BeginPopupModal("EditDeviceMappings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Edit mappings for device '%s' button '%s':",
                        editingDevice->name.c_str(),
                        editingButton->name.c_str());
            ImGui::Separator();

            for (auto& btnName : WiiButtons)
                ImGui::Checkbox(btnName.c_str(), &selectedButtons[btnName]);

            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                editingButton->maps.clear();
                for (auto& pair : selectedButtons)
                    if (pair.second) { MapEntry me; me.value = pair.first; editingButton->maps.push_back(me); }

                std::vector<std::string> mapValues;
                for (auto& me : editingButton->maps) mapValues.push_back(me.value);
                AddCustomMap(editingMfg->name, editingDevice->name, mapValues, editingButton->name, "custom_maps.xml");

                ImGui::CloseCurrentPopup();
                showEditMappingsModal = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); showEditMappingsModal = false; }

            ImGui::EndPopup();
        }

        if (selectedButton >= 0)
        {
            auto& btn = dev.buttons[selectedButton];
            ImGui::Text("Button: %s", btn.name.c_str());
            ImGui::Separator();

            ImGui::Text("Maps:");
            for (auto& map : btn.maps)
                ImGui::BulletText("%s", map.value.c_str());

            ImGui::Separator();

            ImGui::Text("Data:");
            ImGui::InputTextMultiline("##data", (char*)btn.data.c_str(), btn.data.size() + 1,
                                      ImVec2(-FLT_MIN, 120), ImGuiInputTextFlags_ReadOnly);
        }
        else
        {
            ImGui::TextDisabled("Select a button to view details.");
        }
    }
    else
    {
        ImGui::TextDisabled("Select a device.");
    }

    ImGui::EndChild();
    ImGui::End(); // End main window
}
