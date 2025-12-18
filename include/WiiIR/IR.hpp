#pragma once
#ifndef WIIIR_IR_H
#define WIIIR_IR_H

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// STB Usage
#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_PERLIN_IMPLEMENTATION

#ifdef NINTENDOWII
#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/wiilaunch.h>
#else
#define _XML_NO_MSXML
#include <msxml.h>
#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;
#endif

// SDL2 Video Rendering
#include <SDL.h>
#include <SDL_mixer.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "imgui.h"
#endif

// C++ Thingy
#ifdef __cplusplus
extern "C" {
#endif

// Framebuffer and Render Mode.
#ifdef NINTENDOWII
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
#endif

extern float main_scale;
extern SDL_Window* window;
extern SDL_Renderer* renderer;

// GPIO
#ifdef NINTENDOWII
#define _gpio_out_reg (u32 *)0xCD0000C0
#define IRBLAST_PORT 0x100  // Sensor Bar GPIO
#define NWLIGHT_PORT 0x20   // DVD Light GPIO
#endif

// Enum for protocols
enum {
    IR_PROTO_NEC = 0,
    IR_PROTO_NECext,
    IR_PROTO_SAMSUNG32,
    IR_PROTO_SIRC12,
    IR_PROTO_SIRC15,
    IR_PROTO_SIRC20,
    IR_PROTO_RC5,
    IR_PROTO_RC6,
    IR_PROTO_RC6X,
    IR_PROTO_ITT,
    IR_PROTO_JVC,
    IR_PROTO_NRC17,
    IR_PROTO_RECS80,
    IR_PROTO_SHARP,
    IR_PROTO_XSAT,
    IR_PROTO_KASEIKYO,
    IR_PROTO_RAW
};

// Enum for device types
enum {
    IR_DTYPE_TV = 0,
    IR_DTYPE_VCR,
    IR_DTYPE_UNKNOWN
};

// IR remote set mappings.
#ifdef NINTENDOWII
#define IR_REMOTE_KEY_UP       WPAD_BUTTON_UP
#define IR_REMOTE_KEY_DOWN     WPAD_BUTTON_DOWN
#define IR_REMOTE_KEY_LEFT     WPAD_BUTTON_LEFT
#define IR_REMOTE_KEY_RIGHT    WPAD_BUTTON_RIGHT
#define IR_REMOTE_KEY_OK       WPAD_BUTTON_A
#define IR_REMOTE_KEY_BACK     WPAD_BUTTON_B
#define IR_REMOTE_KEY_VP       WPAD_BUTTON_PLUS
#define IR_REMOTE_KEY_VM       WPAD_BUTTON_MINUS
#define IR_REMOTE_KEY_PWR      WPAD_BUTTON_1
#define IR_REMOTE_KEY_INP      WPAD_BUTTON_2
#else
#define IR_REMOTE_KEY_UP       SDLK_UP
#define IR_REMOTE_KEY_DOWN     SDLK_DOWN
#define IR_REMOTE_KEY_LEFT     SDLK_LEFT
#define IR_REMOTE_KEY_RIGHT    SDLK_RIGHT
#define IR_REMOTE_KEY_OK       SDLK_A
#define IR_REMOTE_KEY_BACK     SDLK_B
#define IR_REMOTE_KEY_VP       SDLK_KP_PLUS
#define IR_REMOTE_KEY_VM       SDLK_KP_MINUS
#define IR_REMOTE_KEY_PWR      SDLK_KP_1
#define IR_REMOTE_KEY_INP      SDLK_KP_2
#endif

// Enum for remote mapping.
enum {
    IR_MAP_UP = 0,
    IR_MAP_DOWN,
    IR_MAP_LEFT,
    IR_MAP_RIGHT,
    IR_MAP_OK,
    IR_MAP_BACK,
    IR_MAP_VP,
    IR_MAP_VM,
    IR_MAP_PWR,
    IR_MAP_INP
};

/*
IRDB File Structure:
    Header
    Manufacturer Entry
        Device
            Device Mappings
        Device
            Device Mappings
        ... etc etc, repeat for the number of devices
    Manufacturer Entry
        Device
            Device Mappings.
        ... You get the point now.
*/

// IR Code and Mapping
typedef struct {
    char name[24];
    u16 protocol;
    u32 address;
    u32 command;
    u32 mapped_buttons;
} irdb_mapping_t;

// IRDB Header
#define head_magic_base  ((const char[4]){'I','R','D','B'})
#define mfgr_magic_base  ((const char[4]){'I','M','F','G'})
#define devi_magic_base  ((const char[4]){'I','D','V','C'})

// IRDB Header
//extern const char head_magic_base[4];
//extern const char mfgr_magic_base[4];
//extern const char devi_magic_base[4];

typedef struct {
    char magic[4];      // IRDB
    u8      version;    // IR Database Version
    u32     date;       // Date of code list.
    u32     mfgCount;   // Number of supported manufacturers.
} irdb_header_t;

// InFrared Manufacturer Header.
typedef struct {
    char magic[4];
    char mname[32];
    u8  supported_devicetypes;
    u32 device_count;
} imfg_header_t;

// InFrared Device Entry
typedef struct {
    char magic[4];
    char dname[32];
    u8   device_type;   // Type of Device
    u8   buttons;       // Button Entries
} idvc_entry_t;

// For loading in files.
typedef struct {
    FILE *file;
    irdb_header_t header;
    imfg_header_t **mfg_header;
    idvc_entry_t ***dvc_entry;
    irdb_mapping_t ****mappings;
} irdb_t;

// Kaseikyo
#define IR_KASEIKYO_PERIOD_US 36 // Total period (high + low) for 36 kHz in microseconds
#define IR_KASEIKYO_PERIOD_US_HALF 36 // Half period: 36 µs for each high or low state (50% duty cycle)
#define IR_KASEIKYO_LOGICAL_1 2250 // 2.250mS
#define IR_KASEIKYO_LOGICAL_0 1125 // 1.125mS
#define IR_KASEIKYO_SPACE 560 // 0.560mS

// RC6
#define IR_RC6_PERIOD_US 36 // Total period (high + low) for 36 kHz in microseconds
#define IR_RC6_PERIOD_US_HALF 36 // Half period: 36 µs for each high or low state (50% duty cycle)
#define IR_RC6_LEADER_PULSE_BURST 2666 // 2.666mS
#define IR_RC6_LEADER 889 // 0.889mS
#define IR_RC6_NORMAL 444 // 0.444mS

// RC5
#define IR_RC5_PERIOD_US 36 // Total period (high + low) for 36 kHz in microseconds
#define IR_RC5_PERIOD_US_HALF 36 // Half period: 36 µs for each high or low state (50% duty cycle)
#define IR_RC5_BURST 889 // 0.889mS

// SIRC
typedef enum {
    IR_SIRC_MODE_12 = 0,
    IR_SIRC_MODE_15,
    IR_SIRC_MODE_20
} IRMode_SIRC;

// SIRC constants.
#define IR_SIRC_CAR_FREQ  (float)38.0f
#define IR_SIRC_LOGICAL_1 1200 // 1.20mS
#define IR_SIRC_BURST 600 // 0.60mS
#define IR_SIRC_SPACE 2400 // 2.40mS

// NEC
#define IR_NEC_CAR_FREQ  (float)38.0f // Carrier freq in kHZ.
#define IR_NEC_FULL_FRMT 110000 // 110mS
#define IR_NEC_LOGICAL_1 2250   // 2.25mS
#define IR_NEC_LOGICAL_0 1120   // 1.12mS
#define IR_NEC_BGN_SPACE 9000   // 9.00mS
#define IR_NEC_END_SPACE 4500   // 4.50mS
#define IR_NEC_BURST 560 // 0.56mS

// RCA
#define IR_RCA_CAR_FREQ  (float)56.0f // Carrier freq in kHZ.
#define IR_RCA_LOGICAL_1 2500 // 2.50mS
#define IR_RCA_LOGICAL_0 1500 // 1.50mS
#define IR_RCA_SPACE 4000 // 4.00mS
#define IR_RCA_BURST 500 // 0.50mS

// Samsung32
#define IR_SAMSUNG32_CAR_FREQ  (float)38.0f
#define IR_SAMSUNG32_BGN_SPACE 4500 // 4.50mS
#define IR_SAMSUNG32_BURST     560  // 590uS
#define IR_SAMSUNG32_LOGICAL_1 1690 // 1.69mS
#define IR_SAMSUNG32_LOGICAL_0 560  // 590uS
#define IR_SAMSUNG32_STOP      560  // 590uS

// JVC
#define IR_JVC_CAR_FREQ  (float)38.0f   // 38KHz Carrier
#define IR_JVC_BGN_BREAK 4200           // 4.20mS
#define IR_JVC_BGN_SPACE 8400           // 8.40mS
#define IR_JVC_BURST     526            // 526uS
#define IR_JVC_LOGICAL_1 2100           // 2.10mS
#define IR_JVC_LOGICAL_0 1050           // 1.05mS

// Define constants for Pronto format.
#define PRONTO_FREQCALC_FLOAT_VAL           (float)0.241246  // Frequency calculation constant.
#define PRONTO_SIGTYPE_RAWIR_MODULATED      (u16)0x0000 // Modulated raw IR signal.
#define PRONTO_SIGTYPE_RAWIR_NOTMODULATED   (u16)0x0100 // Non-modulated raw IR signal.

// Base IR
void _IR_SET_GPIO(u32 gpio, u32 value);
void IR_Transmit(float carrier_frequency, int duration_us, float duty_cycle);

// Pronto Codes.
float _pronto_calculate_frequency(uint16_t carrier_code);
void IR_SendPronto(const uint16_t *pronto, size_t length);

// NEC(ext) protocol(s).
void IR_RepeatNEC();
void IR_SendByteNEC(u8 byte, bool inverse);
void IR_SendNECext(u8 adrl, u8 adrm, u8 datal, u8 datam, bool invert_dm);
void IR_SendNEC(u8 adr, u8 data);

// Sony SIRC Protocol
void IR_SendSIRC(IRMode_SIRC mode, u8 address, u16 data);

// Samsung32 Protocol
void IR_SendSamsung32(uint8_t address, uint8_t command);

// JVC Protocol
void IR_SendJVC(uint8_t address, uint8_t command);

// IRDB
void load_json_and_convert(const char *filename);
void run_irdb(const char *filename);

// Utilities
// Enum for standard ANSI VT colors (0-7)
typedef enum {
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    WHITE = 7
} Color;

// Initialize.
void Init();
void Deinit();

// --- All mappable Wii WPAD buttons ---
#ifdef __cplusplus
#ifdef NINTENDOWII
static const std::vector<std::string> WiiButtons = {
    "WPAD_BUTTON_UP",
    "WPAD_BUTTON_DOWN",
    "WPAD_BUTTON_LEFT",
    "WPAD_BUTTON_RIGHT",
    "WPAD_BUTTON_A",
    "WPAD_BUTTON_B",
    "WPAD_BUTTON_1",
    "WPAD_BUTTON_2",
    "WPAD_BUTTON_PLUS",
    "WPAD_BUTTON_MINUS",
    "WPAD_BUTTON_HOME",
    "WPAD_NUNCHUK_C",
    "WPAD_NUNCHUK_Z",
    "WPAD_CLASSIC_BUTTON_A",
    "WPAD_CLASSIC_BUTTON_B",
    "WPAD_CLASSIC_BUTTON_X",
    "WPAD_CLASSIC_BUTTON_Y",
    "WPAD_CLASSIC_BUTTON_Z",
    "WPAD_CLASSIC_BUTTON_L",
    "WPAD_CLASSIC_BUTTON_R",
    "WPAD_CLASSIC_BUTTON_FULL_L",
    "WPAD_CLASSIC_BUTTON_FULL_R",
};
#else
static const std::vector<std::string> WiiButtons = {
    "UP",       // VK_UP
    "DOWN",     // VK_DOWN
    "LEFT",     // VK_LEFT
    "RIGHT",    // VK_RIGHT
    "A",        // 'A' key
    "B",        // 'B' key
    "1",        // '1' key
    "2",        // '2' key
    "PLUS",     // VK_OEM_PLUS
    "MINUS",    // VK_OEM_MINUS
    "ENTER",    // VK_RETURN
    "N",        // 'N' key
    "M",        // 'M' key
    "Q",        // 'Q' key
    "W",        // 'W' key
    "E",        // 'E' key
    "R",        // 'R' key
    "T",        // 'T' key
    "Y",        // 'Y' key
    "U",        // 'U' key
    "I",        // 'I' key
    "O",        // 'O' key
};
#endif

struct MapEntry {
    std::string value;   // e.g., "WPAD_BUTTON_A"
};

struct ButtonEntry {
    std::string name;                // "Power Button"
    std::vector<MapEntry> maps;      // list of maps
    std::string data;                // "NEC:32,122" or RAW codes
};

struct DeviceEntry {
    std::string name;                       // "BeansTV"
    std::vector<ButtonEntry> buttons;
};

struct Manufacturer {
    std::string name;                       // "Toshiba"
    std::vector<DeviceEntry> devices;
};

struct XMLDatabase {
    std::vector<Manufacturer> manufacturers;
};

XMLDatabase LoadXML(const char* filename, const char* customFile = nullptr);
void DrawXMLBrowser(XMLDatabase& db, ImGuiWindowFlags &window_flags);
#endif

// Implementation
void SetupWiiImplementation();

// Helper Functions.
SDL_Texture* GeneratePS3Background(SDL_Renderer* renderer, int width, int height, float frame, int mode);
SDL_Texture* LoadTextureFromArray(SDL_Renderer* renderer, const unsigned char* data, size_t size, int* outWidth, int* outHeight);
SDL_Texture* LoadTextureFromFile(SDL_Renderer* renderer, const char* filePath, int* outWidth, int* outHeight);
void FreeTexture(SDL_Texture* tex);
void StartUI();
void ShutdownUI();
void SetCursorApp(int x, int y);
void SetColors(Color background, Color foreground);
void ClearScreen();
void StopCritical(const char* msg, const char* running_func, const char* occuring_file, int lcall, uint32_t code);
void DrawMSPaintEasterEgg(bool* p_open);

// OSReporter
void setup_osreport_redirection();
void restore_original_cerr();
void restore_original_cout();

// ThemeLoader
bool LoadImGuiThemeFromJSON(const char* filename);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // WIIIR_IR_H