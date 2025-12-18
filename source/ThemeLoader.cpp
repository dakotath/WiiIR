#include "cJSON.h"
#include "WiiIR/IR.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool LoadImGuiThemeFromJSON(const char* filename)
{
    // Read whole file
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonText = buffer.str();

    // Parse JSON
    cJSON* root = cJSON_Parse(jsonText.c_str());
    if (!root)
        return false;

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    auto GetColor = [&](const char* name, ImGuiCol idx) {
        cJSON* c = cJSON_GetObjectItemCaseSensitive(root->child->next, name); // root["Colors"] lookup
        if (c && c->type == cJSON_Array && cJSON_GetArraySize(c) == 4)
        {
            colors[idx] = ImVec4(
                (float)cJSON_GetArrayItem(c, 0)->valuedouble,
                (float)cJSON_GetArrayItem(c, 1)->valuedouble,
                (float)cJSON_GetArrayItem(c, 2)->valuedouble,
                (float)cJSON_GetArrayItem(c, 3)->valuedouble
            );
        }
    };

    auto GetFloat = [&](const char* name, float& output) {
        cJSON* node = cJSON_GetObjectItemCaseSensitive(root->child->next->next, name); // root["Style"] lookup
        if (node && cJSON_IsNumber(node))
            output = (float)node->valuedouble;
    };

    // Get objects
    cJSON* themeNameObj = cJSON_GetObjectItem(root, "ThemeName");
    cJSON* colorsObj = cJSON_GetObjectItem(root, "Colors");
    cJSON* styleObj  = cJSON_GetObjectItem(root, "Style");

    // ----- Print theme name -----
    cJSON* nameItem = cJSON_GetObjectItem(root, "ThemeName");
    if (nameItem && cJSON_IsString(nameItem))
    {
        std::cout << "Loaded ImGui Theme: " << nameItem->valuestring << std::endl;
    }
    else
    {
        std::cout << "Loaded ImGui Theme (Unnamed)" << std::endl;
    }

    if (colorsObj)
    {
        // load colors by name
        GetColor("WindowBg", ImGuiCol_WindowBg);
        GetColor("ChildBg", ImGuiCol_ChildBg);
        GetColor("PopupBg", ImGuiCol_PopupBg);

        GetColor("Border", ImGuiCol_Border);
        GetColor("BorderShadow", ImGuiCol_BorderShadow);

        GetColor("Text", ImGuiCol_Text);
        GetColor("TextDisabled", ImGuiCol_TextDisabled);

        GetColor("FrameBg", ImGuiCol_FrameBg);
        GetColor("FrameBgHovered", ImGuiCol_FrameBgHovered);
        GetColor("FrameBgActive", ImGuiCol_FrameBgActive);

        GetColor("TitleBg", ImGuiCol_TitleBg);
        GetColor("TitleBgActive", ImGuiCol_TitleBgActive);
        GetColor("TitleBgCollapsed", ImGuiCol_TitleBgCollapsed);

        GetColor("Tab", ImGuiCol_Tab);
        GetColor("TabHovered", ImGuiCol_TabHovered);
        GetColor("TabActive", ImGuiCol_TabActive);
        GetColor("TabUnfocused", ImGuiCol_TabUnfocused);
        GetColor("TabUnfocusedActive", ImGuiCol_TabUnfocusedActive);

        GetColor("Button", ImGuiCol_Button);
        GetColor("ButtonHovered", ImGuiCol_ButtonHovered);
        GetColor("ButtonActive", ImGuiCol_ButtonActive);

        GetColor("Header", ImGuiCol_Header);
        GetColor("HeaderHovered", ImGuiCol_HeaderHovered);
        GetColor("HeaderActive", ImGuiCol_HeaderActive);

        GetColor("CheckMark", ImGuiCol_CheckMark);

        GetColor("SliderGrab", ImGuiCol_SliderGrab);
        GetColor("SliderGrabActive", ImGuiCol_SliderGrabActive);

        GetColor("ScrollbarBg", ImGuiCol_ScrollbarBg);
        GetColor("ScrollbarGrab", ImGuiCol_ScrollbarGrab);
        GetColor("ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered);
        GetColor("ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive);

        GetColor("Separator", ImGuiCol_Separator);
        GetColor("SeparatorHovered", ImGuiCol_SeparatorHovered);
        GetColor("SeparatorActive", ImGuiCol_SeparatorActive);

        GetColor("ResizeGrip", ImGuiCol_ResizeGrip);
        GetColor("ResizeGripHovered", ImGuiCol_ResizeGripHovered);
        GetColor("ResizeGripActive", ImGuiCol_ResizeGripActive);

        GetColor("TableBorderStrong", ImGuiCol_TableBorderStrong);
        GetColor("TableBorderLight", ImGuiCol_TableBorderLight);
        GetColor("TableRowBg", ImGuiCol_TableRowBg);
        GetColor("TableRowBgAlt", ImGuiCol_TableRowBgAlt);

        GetColor("TextSelectedBg", ImGuiCol_TextSelectedBg);
        GetColor("DragDropTarget", ImGuiCol_DragDropTarget);
        GetColor("NavHighlight", ImGuiCol_NavHighlight);
        GetColor("NavWindowingHighlight", ImGuiCol_NavWindowingHighlight);
        GetColor("NavWindowingDimBg", ImGuiCol_NavWindowingDimBg);
        GetColor("ModalWindowDimBg", ImGuiCol_ModalWindowDimBg);
    }

    if (styleObj)
    {
        GetFloat("WindowRounding", style.WindowRounding);
        GetFloat("FrameRounding", style.FrameRounding);
        GetFloat("ScrollbarRounding", style.ScrollbarRounding);
        GetFloat("GrabRounding", style.GrabRounding);
        GetFloat("TabRounding", style.TabRounding);
        GetFloat("WindowBorderSize", style.WindowBorderSize);
        GetFloat("FrameBorderSize", style.FrameBorderSize);
        GetFloat("PopupBorderSize", style.PopupBorderSize);
    }

    cJSON_Delete(root);
    return true;
}
