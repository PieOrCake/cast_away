#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <mutex>
#include "nexus/Nexus.h"
#include "imgui.h"
#include <nlohmann/json.hpp>
#include "FishData.h"
#include "TileCache.h"
#include "MapPanel.h"

#define V_MAJOR 0
#define V_MINOR 1
#define V_BUILD 0
#define V_REVISION 0
#define QA_ID          "QA_CAST_AWAY"
#define TEX_ICON       "TEX_CAST_AWAY_ICON"
#define TEX_ICON_HOVER "TEX_CAST_AWAY_ICON_HOVER"

static AddonAPI_t*       APIDefs  = nullptr;
static AddonDefinition_t AddonDef{};
static bool g_WindowVisible  = false;
static bool g_OverlayVisible = false;
static bool g_ShowQAIcon     = true;

void AddonLoad(AddonAPI_t* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    AddonDef.Signature        = 0xCA57A000;
    AddonDef.APIVersion       = NEXUS_API_VERSION;
    AddonDef.Name             = "Cast Away";
    AddonDef.Version.Major    = V_MAJOR;
    AddonDef.Version.Minor    = V_MINOR;
    AddonDef.Version.Build    = V_BUILD;
    AddonDef.Version.Revision = V_REVISION;
    AddonDef.Author           = "PieOrCake.7635";
    AddonDef.Description      = "Fishing companion: searchable fish database with time-of-day, bait info, and interactive map.";
    AddonDef.Load             = AddonLoad;
    AddonDef.Unload           = AddonUnload;
    AddonDef.Flags            = AF_None;
    AddonDef.Provider         = UP_GitHub;
    AddonDef.UpdateLink       = "https://github.com/PieOrCake/cast_away";
    return &AddonDef;
}

void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void*(*)(size_t, void*))APIDefs->ImguiMalloc,
        (void(*)(void*, void*))APIDefs->ImguiFree);
    APIDefs->GUI_Register(RT_Render, AddonRender);
    APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);
    APIDefs->InputBinds_RegisterWithString("KB_CAST_AWAY_TOGGLE", [](const char*, bool rel) {
        if (!rel) g_WindowVisible = !g_WindowVisible;
    }, "CTRL+SHIFT+F");
    APIDefs->GUI_RegisterCloseOnEscape("Cast Away", &g_WindowVisible);
}

void AddonUnload() {
    APIDefs->GUI_Deregister(AddonRender);
    APIDefs->GUI_Deregister(AddonOptions);
    APIDefs->InputBinds_Deregister("KB_CAST_AWAY_TOGGLE");
}

void AddonRender() {
    if (!g_WindowVisible) return;
    ImGui::Begin("Cast Away", &g_WindowVisible);
    ImGui::Text("Cast Away — stub");
    ImGui::End();
}

void AddonOptions() {
    ImGui::Text("Cast Away Options — stub");
}
