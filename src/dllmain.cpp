#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include "nexus/Nexus.h"
#include "imgui.h"
#include <nlohmann/json.hpp>
#include "FishData.h"
#include "MapPanel.h"
#include "IconManager.h"
#include "PriceCache.h"
#include "AchievementTracker.h"

#define V_MAJOR    0
#define V_MINOR    1
#define V_BUILD    0
#define V_REVISION 0
#define QA_ID          "QA_CAST_AWAY"
#define TEX_ICON       "TEX_CAST_AWAY_ICON"
#define TEX_ICON_HOVER "TEX_CAST_AWAY_ICON_HOVER"

// ---------------------------------------------------------------------------
// Embedded QA icon data (32x32 RGBA PNG, fishing theme)
// ---------------------------------------------------------------------------
static const unsigned char ICON_NORMAL[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a, 0xf4, 0x00, 0x00, 0x01,
  0x5d, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xcd, 0x97, 0x2d, 0x72, 0xc3,
  0x30, 0x10, 0x46, 0x77, 0x77, 0x72, 0x84, 0xdc, 0x20, 0xc4, 0xa4, 0x38,
  0x05, 0x25, 0xc9, 0x09, 0x4a, 0x4a, 0x4c, 0x4a, 0x72, 0x83, 0x92, 0x94,
  0x18, 0x98, 0x24, 0x24, 0x37, 0x30, 0x29, 0x09, 0x09, 0xe9, 0x09, 0x12,
  0x52, 0xd0, 0xe0, 0x10, 0x93, 0xde, 0xa0, 0x77, 0x68, 0x66, 0x67, 0xa2,
  0x19, 0x55, 0x91, 0x56, 0x92, 0x2d, 0x39, 0xf9, 0x80, 0xed, 0xb1, 0x25,
  0xbd, 0x27, 0x8f, 0x46, 0x3f, 0x00, 0x37, 0x0e, 0xc6, 0x56, 0x28, 0x16,
  0x6f, 0x7f, 0xd2, 0xf7, 0xb6, 0xd9, 0x44, 0xb5, 0x89, 0xb1, 0xe0, 0xdf,
  0xf6, 0x24, 0x96, 0x1b, 0x17, 0x0f, 0x51, 0x22, 0xd8, 0x05, 0xfc, 0x54,
  0xbe, 0xc2, 0xd7, 0xf6, 0x23, 0x89, 0x08, 0xfa, 0xe0, 0xae, 0x1e, 0xb3,
  0x04, 0x27, 0x44, 0x44, 0x92, 0xa0, 0x2e, 0x70, 0x1d, 0xac, 0x44, 0x5c,
  0xe1, 0x36, 0xa4, 0x71, 0x43, 0x5d, 0xe0, 0x66, 0x58, 0x42, 0x12, 0x91,
  0x24, 0xa8, 0x0f, 0xdc, 0xfc, 0xfd, 0x5d, 0x24, 0x30, 0x06, 0xbe, 0xa8,
  0x6b, 0xeb, 0xfb, 0xb6, 0xfd, 0xf1, 0xca, 0xb9, 0xc6, 0xc4, 0xc8, 0x49,
  0x13, 0x80, 0x66, 0x8a, 0x62, 0x72, 0xfd, 0xee, 0x52, 0xb7, 0xa9, 0x2a,
  0xb1, 0x2e, 0xda, 0x7a, 0x1f, 0x0a, 0x8e, 0x89, 0x2e, 0xa2, 0xff, 0x05,
  0x32, 0x0b, 0xe6, 0x80, 0x4b, 0xed, 0x92, 0xde, 0xfb, 0x5c, 0x70, 0x53,
  0x42, 0x1f, 0x90, 0xe4, 0x33, 0xcc, 0x25, 0xa1, 0x42, 0x7c, 0x79, 0x2e,
  0x5f, 0x06, 0x81, 0xdb, 0x24, 0x68, 0xb9, 0xff, 0x16, 0x57, 0xb7, 0x3e,
  0x59, 0xcd, 0xa6, 0xde, 0x32, 0x94, 0x1b, 0xee, 0x93, 0xa0, 0x5c, 0x02,
  0xef, 0x87, 0xe3, 0xbf, 0xbb, 0x53, 0x60, 0x3d, 0x7f, 0xc4, 0xdc, 0x12,
  0xa2, 0x00, 0x00, 0xc0, 0xe7, 0x76, 0x07, 0x43, 0x46, 0x9f, 0x94, 0x88,
  0x2f, 0x3c, 0x2b, 0x0d, 0x25, 0xc1, 0x70, 0xe7, 0x4c, 0xd8, 0x78, 0xe6,
  0xed, 0x14, 0x70, 0x33, 0xa4, 0x1e, 0xd8, 0x88, 0xcd, 0x72, 0x49, 0xa8,
  0x76, 0x83, 0x56, 0xc3, 0xe6, 0x52, 0x38, 0xc5, 0xec, 0x18, 0xbc, 0x1a,
  0x86, 0xec, 0x09, 0x42, 0x84, 0x24, 0xa0, 0x6d, 0x7f, 0x88, 0x29, 0xb6,
  0x64, 0x21, 0x71, 0x6d, 0x4e, 0xc9, 0x56, 0x58, 0x8d, 0x87, 0xdc, 0x70,
  0xa7, 0x40, 0x4a, 0x09, 0xdf, 0xb6, 0x1c, 0x53, 0x9d, 0x88, 0x6c, 0xe0,
  0xde, 0x07, 0x93, 0xbb, 0x38, 0x9a, 0x81, 0x91, 0xd4, 0x87, 0xd3, 0x9b,
  0xe7, 0x0c, 0xb7, 0x60, 0xc7, 0x57, 0x90, 0x6a, 0x26, 0xa4, 0x00, 0x00,
  0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
static const unsigned int ICON_NORMAL_size = 406;

static const unsigned char ICON_HOVER[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a, 0xf4, 0x00, 0x00, 0x01,
  0x4d, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xcd, 0x97, 0x31, 0x12, 0xc2,
  0x20, 0x10, 0x00, 0xb9, 0x1b, 0x3f, 0xa1, 0x85, 0x4d, 0x7c, 0x41, 0x6a,
  0x6b, 0x1b, 0x2d, 0xfc, 0x01, 0xf9, 0x88, 0x29, 0xf4, 0x23, 0xc9, 0x0f,
  0x52, 0x68, 0x63, 0x6d, 0xed, 0x0b, 0xb4, 0xb1, 0xd0, 0x67, 0xe8, 0x90,
  0x31, 0x33, 0x84, 0xc0, 0x01, 0x09, 0x44, 0xaf, 0xd1, 0xd1, 0xe3, 0x76,
  0x61, 0x18, 0x38, 0x18, 0xfb, 0x71, 0x80, 0xef, 0x80, 0x6d, 0x71, 0x7c,
  0x53, 0xff, 0x57, 0xd9, 0xc6, 0xab, 0x26, 0xf8, 0x82, 0xaf, 0xe7, 0x13,
  0x99, 0x97, 0xae, 0xd6, 0x5e, 0x22, 0xd0, 0x07, 0xbc, 0xe5, 0x9c, 0x55,
  0x65, 0x19, 0x44, 0x04, 0x6c, 0x70, 0xd3, 0x8c, 0x85, 0x44, 0x0d, 0x70,
  0x10, 0xa1, 0x24, 0xb0, 0x0f, 0x5c, 0x06, 0x37, 0x22, 0xa6, 0x10, 0x35,
  0xa8, 0x7d, 0x83, 0x7d, 0xe0, 0x9d, 0x7c, 0xce, 0x49, 0x11, 0x4a, 0x02,
  0x87, 0xc0, 0xd5, 0xe5, 0xef, 0x23, 0x01, 0x3e, 0xf0, 0xfd, 0x7e, 0xa7,
  0x2f, 0x7e, 0x7f, 0x58, 0xe5, 0x4c, 0x7b, 0x62, 0x62, 0xa4, 0x11, 0xc0,
  0x4e, 0xd1, 0x64, 0xde, 0xfd, 0xed, 0x3b, 0x36, 0xcf, 0x0f, 0xe4, 0x58,
  0xd0, 0xcd, 0xde, 0x15, 0xec, 0x13, 0xb2, 0x88, 0xbc, 0x0a, 0xa8, 0x26,
  0xc6, 0x80, 0x53, 0x75, 0x51, 0x9e, 0x7d, 0x2c, 0xb8, 0x2a, 0x21, 0x6f,
  0x48, 0xb4, 0x19, 0xc6, 0x92, 0x68, 0xaf, 0xc0, 0x32, 0x1d, 0x05, 0xae,
  0x93, 0xc0, 0xe2, 0xf6, 0x24, 0x6f, 0xb7, 0x21, 0xc1, 0x93, 0xa9, 0x35,
  0x07, 0x63, 0xc3, 0x6d, 0x12, 0x18, 0x4b, 0xa0, 0xbc, 0xbf, 0x5a, 0x9f,
  0x46, 0x81, 0x6c, 0x31, 0x83, 0xd8, 0x12, 0xa4, 0x00, 0x13, 0xc7, 0xe6,
  0xe5, 0xca, 0xc6, 0x0c, 0xf9, 0x50, 0xc2, 0x5a, 0x20, 0xdb, 0xc0, 0x58,
  0x12, 0x02, 0x6e, 0x3c, 0x09, 0x73, 0xcb, 0xb9, 0x1d, 0x02, 0xae, 0x06,
  0x36, 0x5f, 0x84, 0x91, 0x30, 0x8b, 0x25, 0xd1, 0xd4, 0x75, 0xba, 0x0d,
  0xf3, 0x6f, 0x72, 0x88, 0xd3, 0xd1, 0xf9, 0x36, 0x74, 0xe9, 0x09, 0x5c,
  0x84, 0x28, 0xa0, 0xae, 0x3f, 0x84, 0x10, 0x2d, 0x99, 0x4b, 0x98, 0x9a,
  0x53, 0xd4, 0x25, 0x37, 0xfb, 0x21, 0x36, 0xdc, 0x28, 0x10, 0x52, 0xc2,
  0xd6, 0x96, 0x43, 0xa8, 0x17, 0x91, 0x0e, 0x3c, 0xf8, 0x61, 0xf2, 0x17,
  0x4f, 0x33, 0xa6, 0x44, 0xe8, 0xc7, 0xe9, 0xcf, 0xe3, 0x03, 0x48, 0x98,
  0xc7, 0x75, 0xb8, 0xf3, 0xf9, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
  0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
static const unsigned int ICON_HOVER_size = 390;

// ---------------------------------------------------------------------------
// MumbleLink structs (packed, mirrors GW2 memory layout)
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct GW2Context {
    uint8_t  serverAddress[28];
    uint32_t mapId;
    uint32_t mapType;
};
struct MumbleLinkedMem {
    uint32_t uiVersion;
    uint32_t uiTick;
    float    fAvatarPosition[3];
    float    fAvatarFront[3];
    wchar_t  name[256];
    float    fCameraPosition[3];
    float    fCameraFront[3];
    wchar_t  identity[256];
    uint32_t context_len;
    uint8_t  context[256];
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void AddonLoad(AddonAPI_t* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static AddonAPI_t*       APIDefs  = nullptr;
static AddonDefinition_t AddonDef{};
static bool g_WindowVisible  = false;
static bool g_OverlayVisible = false;
static bool g_ShowQAIcon     = true;

static MapPanel g_MapPanel;

static int  g_SelectedFish    = -1;
static char g_SearchBuf[128]  = {};
static int  g_FilterBait      = 0;
static int  g_FilterTime      = 0;
static int  g_FilterWater     = -1;
static bool g_ShowCurrentOnly = false;
static bool g_MissingOnly     = false;
static std::string g_FilterMap;
static std::string g_CurrentMapName;

static std::vector<std::string> g_Favourites;
static std::mutex               g_FavMutex;

static bool g_SwitchToDatabase = false;

static std::vector<int> g_SortedFishIndices;

static const int   HOLE_RESPAWN_SECONDS = 600;
static const int   HOLE_DWELL_SECONDS   = 30;
static const float HOLE_PROXIMITY_M     = 50.f;

struct HoleDwellState {
    std::chrono::steady_clock::time_point enteredAt;
    bool inside = false;
};
static std::unordered_map<int, HoleDwellState>                        g_HoleDwell;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_HoleRespawn;
static std::mutex g_RespawnMu;

static int g_NotifyLeadSeconds = 180;
struct Toast {
    std::string message;
    std::string subtext;
    float       expiresAt;
};
static std::vector<Toast> g_Toasts;
static std::mutex         g_ToastMu;
static std::unordered_map<int, uint32_t> g_LastNotifiedCycle;

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
static std::string SettingsPath() {
    return std::string(APIDefs->Paths_GetAddonDirectory("cast_away")) + "/settings.json";
}

static void SaveSettings() {
    try {
        nlohmann::json j;
        {
            std::lock_guard<std::mutex> lk(g_FavMutex);
            j["favourites"] = g_Favourites;
        }
        j["overlay_visible"]     = g_OverlayVisible;
        j["show_qa_icon"]        = g_ShowQAIcon;
        j["notify_lead_seconds"] = g_NotifyLeadSeconds;

        std::string path = SettingsPath();
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path());
        std::ofstream f(path);
        if (f.is_open()) f << j.dump(2);
    } catch (...) {}
}

static void LoadSettings() {
    try {
        std::ifstream f(SettingsPath());
        if (!f.is_open()) return;
        nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
        if (j.is_discarded()) return;

        if (j.contains("favourites") && j["favourites"].is_array()) {
            std::lock_guard<std::mutex> lk(g_FavMutex);
            g_Favourites.clear();
            for (auto& v : j["favourites"])
                if (v.is_string()) g_Favourites.push_back(v.get<std::string>());
        }
        if (j.contains("overlay_visible"))     g_OverlayVisible    = j["overlay_visible"].get<bool>();
        if (j.contains("show_qa_icon"))        g_ShowQAIcon        = j["show_qa_icon"].get<bool>();
        if (j.contains("notify_lead_seconds")) g_NotifyLeadSeconds = j["notify_lead_seconds"].get<int>();
    } catch (...) {}
}

// ---------------------------------------------------------------------------
// Filter helpers
// ---------------------------------------------------------------------------
static bool FishMatchesFilter(int fishIdx) {
    const Fish& f = FISH_TABLE[fishIdx];

    // Name search (case-insensitive)
    if (g_SearchBuf[0]) {
        std::string haystack = f.name;
        std::string needle   = g_SearchBuf;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
        std::transform(needle.begin(),   needle.end(),   needle.begin(),   ::tolower);
        if (haystack.find(needle) == std::string::npos) return false;
    }

    // Bait filter
    if (g_FilterBait > 0 && (int)f.bait != g_FilterBait) return false;

    // Time filter
    if (g_ShowCurrentOnly) {
        TimeOfDay cur = GetCurrentTimeOfDay();
        if (f.time != TimeOfDay::Any && f.time != cur) return false;
    } else if (g_FilterTime > 0 && f.time != TimeOfDay::Any && (int)f.time != g_FilterTime) {
        return false;
    }

    // Water type
    if (g_FilterWater >= 0 && (int)f.water != g_FilterWater) return false;

    // Map filter
    if (!g_FilterMap.empty() && g_FilterMap != std::string(f.map ? f.map : "")) return false;

    // Missing only — hide fish already caught
    if (g_MissingOnly && g_AchTracker.hoarded && g_AchTracker.IsCaught(fishIdx)) return false;

    return true;
}

static bool IsFavourite(const char* name) {
    std::lock_guard<std::mutex> lk(g_FavMutex);
    return std::find(g_Favourites.begin(), g_Favourites.end(), name) != g_Favourites.end();
}

static void ToggleFavourite(const char* name) {
    std::lock_guard<std::mutex> lk(g_FavMutex);
    auto it = std::find(g_Favourites.begin(), g_Favourites.end(), name);
    if (it != g_Favourites.end()) g_Favourites.erase(it);
    else                          g_Favourites.push_back(name);
}

// ---------------------------------------------------------------------------
// Day/Night timeline bar
// ---------------------------------------------------------------------------
static void RenderDayNightBar(float windowWidth) {
    static const float BAR_H   = 38.f;
    static const float TICK_H  = 18.f;
    static const float TOTAL_H = BAR_H + TICK_H;

    ImDrawList* dl     = ImGui::GetWindowDrawList();
    ImVec2      origin = ImGui::GetCursorScreenPos();

    // Gradient colour stops: {tyrianHour, R, G, B}
    struct Stop { float h; uint8_t r, g, b; };
    static const Stop stops[] = {
        { 0.00f, 10,  10,  30  },
        { 1.67f, 45,  27,  78  },
        { 3.33f, 196, 107, 26  },
        { 5.00f, 88,  153, 212 },
        {12.00f, 135, 206, 235 },
        {15.00f, 88,  153, 212 },
        {16.67f, 212, 112, 42  },
        {18.00f, 26,  10,  46  },
        {24.00f, 10,  10,  30  },
    };
    static const int NSTOPS = (int)(sizeof(stops) / sizeof(stops[0]));

    for (int i = 0; i < NSTOPS - 1; i++) {
        float x0 = origin.x + (stops[i].h   / 24.f) * windowWidth;
        float x1 = origin.x + (stops[i+1].h / 24.f) * windowWidth;
        ImU32 c0 = IM_COL32(stops[i].r,   stops[i].g,   stops[i].b,   255);
        ImU32 c1 = IM_COL32(stops[i+1].r, stops[i+1].g, stops[i+1].b, 255);
        dl->AddRectFilledMultiColor(
            {x0, origin.y}, {x1, origin.y + BAR_H},
            c0, c1, c1, c0);
    }

    // Current time / phase
    float tyrHour = GetTyrianHour();
    TimeOfDay phase = GetCurrentTimeOfDay();
    uint32_t secLeft = SecondsUntilNextSlot();

    // Diamond marker at current hour
    float dx = origin.x + (tyrHour / 24.f) * windowWidth;
    float dy = origin.y + BAR_H - 4.f;
    float ds = 6.f;
    dl->AddQuadFilled(
        {dx,      dy - ds},
        {dx + ds, dy      },
        {dx,      dy + ds },
        {dx - ds, dy      },
        IM_COL32(255, 255, 255, 220));
    dl->AddQuad(
        {dx,      dy - ds},
        {dx + ds, dy      },
        {dx,      dy + ds },
        {dx - ds, dy      },
        IM_COL32(0, 0, 0, 180), 1.f);

    // Left label: "HH:MM  PhaseName"
    char timeLabel[64];
    {
        uint32_t ts = GetTyrianSeconds();
        uint32_t hh = ts / 180;
        uint32_t mm = (ts % 180) * 60 / 180;
        snprintf(timeLabel, sizeof(timeLabel), "%02u:%02u  %s", hh, mm, TimeOfDayName(phase));
    }
    dl->AddText({origin.x + 4.f, origin.y + 4.f},
                IM_COL32(255, 255, 255, 230), timeLabel);

    // Right label: "Xm XXs"
    char untilLabel[32];
    snprintf(untilLabel, sizeof(untilLabel), "%um %02us", secLeft / 60, secLeft % 60);
    ImVec2 tsz = ImGui::CalcTextSize(untilLabel);
    dl->AddText({origin.x + windowWidth - tsz.x - 4.f, origin.y + 4.f},
                IM_COL32(255, 255, 255, 200), untilLabel);

    // Tick strip background
    dl->AddRectFilled(
        {origin.x, origin.y + BAR_H},
        {origin.x + windowWidth, origin.y + TOTAL_H},
        IM_COL32(15, 15, 20, 220));

    // Tick marks at 0, 6, 12, 18, 24
    static const float tickHours[]  = {0.f, 6.f, 12.f, 18.f, 24.f};
    static const char* tickLabels[] = {"0",  "6",  "12",  "18",  "24"};
    for (int i = 0; i < 5; i++) {
        float tx = origin.x + (tickHours[i] / 24.f) * windowWidth;
        dl->AddLine({tx, origin.y + BAR_H},
                    {tx, origin.y + TOTAL_H - 4.f},
                    IM_COL32(180, 180, 180, 160), 1.f);
        ImVec2 lsz = ImGui::CalcTextSize(tickLabels[i]);
        float lx = tx - lsz.x * 0.5f;
        if (lx < origin.x) lx = origin.x;
        if (lx + lsz.x > origin.x + windowWidth)
            lx = origin.x + windowWidth - lsz.x;
        dl->AddText({lx, origin.y + BAR_H + 2.f},
                    IM_COL32(200, 200, 200, 200), tickLabels[i]);
    }

    // Advance ImGui cursor past the bar
    ImGui::Dummy({windowWidth, TOTAL_H});
}

// ---------------------------------------------------------------------------
// Conditions overlay (small HUD)
// ---------------------------------------------------------------------------
static void RenderOverlay() {
    if (!g_OverlayVisible) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x - 180.f, 40.f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({170.f, 60.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##CastAwayOverlay", nullptr, flags)) {
        TimeOfDay phase = GetCurrentTimeOfDay();
        uint32_t  secs  = SecondsUntilNextSlot();
        ImGui::Text("GW2 Time: %s", TimeOfDayName(phase));
        ImGui::Text("Next: %um %02us", secs / 60, secs % 60);
    }
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Toast helpers
// ---------------------------------------------------------------------------
static void PushToast(const std::string& message, const std::string& subtext,
                      float durationSec = 6.f) {
    float now = (float)ImGui::GetTime();
    std::lock_guard<std::mutex> lk(g_ToastMu);
    for (auto& t : g_Toasts)
        if (t.message == message && t.expiresAt > now) return;
    g_Toasts.push_back({message, subtext, now + durationSec});
}

// ---------------------------------------------------------------------------
// Toast render
// ---------------------------------------------------------------------------
static void RenderToasts() {
    float now = (float)ImGui::GetTime();
    std::lock_guard<std::mutex> lk(g_ToastMu);

    g_Toasts.erase(
        std::remove_if(g_Toasts.begin(), g_Toasts.end(),
                       [now](const Toast& t){ return t.expiresAt <= now; }),
        g_Toasts.end());

    if (g_Toasts.empty()) return;

    ImGuiIO& io = ImGui::GetIO();
    float yBase = io.DisplaySize.y - 20.f;

    for (int i = (int)g_Toasts.size() - 1; i >= 0; i--) {
        const Toast& t = g_Toasts[i];
        float remaining = t.expiresAt - now;
        float alpha = (remaining < 1.f) ? remaining : 1.f;

        ImVec2 sz{280.f, t.subtext.empty() ? 36.f : 52.f};
        yBase -= sz.y + 4.f;

        ImGui::SetNextWindowPos({12.f, yBase}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.80f * alpha);

        char wid[32];
        snprintf(wid, sizeof(wid), "##toast%d", i);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoInputs        |
            ImGuiWindowFlags_NoNav           |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        if (ImGui::Begin(wid, nullptr, flags)) {
            ImGui::TextUnformatted(t.message.c_str());
            if (!t.subtext.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 220));
                ImGui::TextUnformatted(t.subtext.c_str());
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

// ---------------------------------------------------------------------------
// Hole dwell tracking
// ---------------------------------------------------------------------------
static void UpdateHoleDwellTimers(int mapId, float gx, float gz) {
    auto now = std::chrono::steady_clock::now();
    for (int i = 0; i < HOLE_COUNT; i++) {
        const FishingHole& h = HOLE_TABLE[i];
        if ((int)h.mapId != mapId) continue;
        if (h.game_x == 0.f && h.game_z == 0.f) continue;

        float dx   = gx - h.game_x;
        float dz   = gz - h.game_z;
        float dist = sqrtf(dx*dx + dz*dz);
        bool  inside = (dist < HOLE_PROXIMITY_M);

        HoleDwellState& ds = g_HoleDwell[i];
        if (inside && !ds.inside) {
            ds.inside    = true;
            ds.enteredAt = now;
        } else if (!inside && ds.inside) {
            ds.inside = false;
            auto dwellSecs = std::chrono::duration_cast<std::chrono::seconds>(
                now - ds.enteredAt).count();
            if (dwellSecs >= HOLE_DWELL_SECONDS) {
                std::lock_guard<std::mutex> lk(g_RespawnMu);
                g_HoleRespawn[i] = now;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Time window notifications
// ---------------------------------------------------------------------------
static void CheckTimeWindowNotifications() {
    static auto lastCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck).count() < 1)
        return;
    lastCheck = now;

    uint32_t currentCycle = GetTyrianSeconds() / 60;

    std::vector<std::string> favsCopy;
    {
        std::lock_guard<std::mutex> lk(g_FavMutex);
        favsCopy = g_Favourites;
    }

    for (int i = 0; i < FISH_COUNT; i++) {
        const Fish& f = FISH_TABLE[i];
        if (f.time == TimeOfDay::Any) continue;

        bool fav = false;
        for (auto& n : favsCopy) if (n == f.name) { fav = true; break; }
        if (!fav) continue;

        uint32_t secs = SecondsUntilPhase(f.time);
        if (secs > (uint32_t)g_NotifyLeadSeconds) continue;

        auto it = g_LastNotifiedCycle.find(i);
        if (it != g_LastNotifiedCycle.end() && it->second == currentCycle) continue;
        g_LastNotifiedCycle[i] = currentCycle;

        char msg[128], sub[128];
        snprintf(msg, sizeof(msg), "%s window starting soon!", TimeOfDayName(f.time));
        snprintf(sub, sizeof(sub), "%s \xe2\x80\x94 %um %02us", f.name, secs / 60, secs % 60);
        PushToast(msg, sub);
    }
}

// ---------------------------------------------------------------------------
// Nearby hole HUD
// ---------------------------------------------------------------------------
static void CheckNearbyHoles(int mapId, float gx, float gz) {
    if (mapId == 0) return;
    TimeOfDay curTime = GetCurrentTimeOfDay();

    for (int i = 0; i < HOLE_COUNT; i++) {
        const FishingHole& h = HOLE_TABLE[i];
        if ((int)h.mapId != mapId) continue;
        if (h.game_x == 0.f && h.game_z == 0.f) continue;

        float dx   = gx - h.game_x;
        float dz   = gz - h.game_z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist >= HOLE_PROXIMITY_M) continue;

        // Collect catchable fish
        std::vector<const Fish*> catchable;
        for (int fi = 0; fi < (int)h.fishCount; fi++) {
            uint16_t fid = h.fishIds[fi];
            if (fid >= (uint16_t)FISH_COUNT) continue;
            const Fish& ff = FISH_TABLE[fid];
            if (ff.time == TimeOfDay::Any || ff.time == curTime)
                catchable.push_back(&ff);
        }

        ImGuiIO& io = ImGui::GetIO();
        float w      = 240.f;
        float lineH  = ImGui::GetTextLineHeightWithSpacing();
        float winH   = 28.f + lineH * (float)(catchable.empty() ? 1 : (int)catchable.size());
        ImGui::SetNextWindowPos(
            {io.DisplaySize.x * 0.5f - w * 0.5f, io.DisplaySize.y * 0.35f},
            ImGuiCond_Always);
        ImGui::SetNextWindowSize({w, winH}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.75f);

        char wid[32];
        snprintf(wid, sizeof(wid), "##hole%d", i);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoInputs        |
            ImGuiWindowFlags_NoNav           |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin(wid, nullptr, flags)) {
            ImGui::TextColored({1.f, 0.85f, 0.3f, 1.f}, "%s", h.name);
            if (catchable.empty()) {
                ImGui::TextDisabled("No fish active now");
            } else {
                for (const Fish* fp : catchable)
                    ImGui::BulletText("%s", fp->name);
            }
        }
        ImGui::End();
        break; // Show only the nearest hole
    }
}

// ---------------------------------------------------------------------------
// Bait shopping list (Task 15)
// ---------------------------------------------------------------------------
static void RenderBaitShoppingList() {
    if (!g_AchTracker.hoarded) {
        ImGui::TextDisabled("Requires Hoard & Seek for missing fish data.");
        return;
    }
    std::unordered_map<int, int> baitCount;
    for (int i = 0; i < FISH_COUNT; ++i) {
        if (g_AchTracker.IsCaught(i)) continue;
        baitCount[(int)FISH_TABLE[i].bait]++;
    }
    if (baitCount.empty()) {
        ImGui::TextColored({0.3f,0.9f,0.3f,1.f}, "All fish caught!");
        return;
    }
    if (ImGui::BeginTable("BaitList", 2,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Bait", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Fish needed", ImGuiTableColumnFlags_WidthFixed, 80.f);
        ImGui::TableHeadersRow();
        for (int b = 0; b < BAIT_COUNT; ++b) {
            auto it = baitCount.find(b);
            if (it == baitCount.end()) continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(BAIT_NAMES[b]);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", it->second);
        }
        ImGui::EndTable();
    }
    if (ImGui::Button("Copy as text")) {
        std::string out = "Bait needed for missing fish:\n";
        for (int b = 0; b < BAIT_COUNT; ++b) {
            auto it = baitCount.find(b);
            if (it == baitCount.end()) continue;
            out += "  " + std::string(BAIT_NAMES[b]) + " x" + std::to_string(it->second) + "\n";
        }
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, out.size()+1);
            if (h) { memcpy(GlobalLock(h), out.c_str(), out.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); }
            CloseClipboard();
        }
    }
}

// ---------------------------------------------------------------------------
// Fish details panel
// ---------------------------------------------------------------------------
static void RenderFishDetails(int fishIdx) {
    if (fishIdx < 0 || fishIdx >= FISH_COUNT) return;
    const Fish& f = FISH_TABLE[fishIdx];

    // Request TP prices
    g_Prices.Request(f.itemId, f.filletItemId);

    // Fish icon 48x48
    if (f.itemId != 0) {
        Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
        if (tex && tex->Resource) {
            ImGui::Image((ImTextureID)(uintptr_t)tex->Resource, {48.f, 48.f});
        } else {
            CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
            ImGui::Dummy({48.f, 48.f});
        }
        ImGui::SameLine();
    }

    ImGui::BeginGroup();
    ImGui::TextColored({1.f, 0.85f, 0.3f, 1.f}, "%s", f.name);
    if (f.region && f.region[0])
        ImGui::TextDisabled("%s", f.region);
    ImGui::EndGroup();

    ImGui::Separator();

    // Helper lambda for detail rows
    auto row = [](const char* label, const char* value) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("%s", label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(value ? value : "-");
    };

    if (ImGui::BeginTable("##FishInfo", 2,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg,
            {-1.f, 0.f})) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        row("Map",        f.map);
        row("Water",      WaterTypeName(f.water));
        row("Bait",       BAIT_NAMES[(int)f.bait]);
        row("Time",       TimeOfDayName(f.time));
        row("Collection", f.collection ? f.collection : "-");
        if (g_AchTracker.hoarded) {
            row("Caught", g_AchTracker.IsCaught(g_SelectedFish) ? "\xe2\x9c\x93" : "\xe2\x9c\x97");
        } else {
            row("Caught", "\xe2\x80\x94");
        }

        if (f.masteryRequired && f.masteryRequired[0]) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Mastery");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored({1.f, 0.6f, 0.2f, 1.f}, "%s", f.masteryRequired);
        }

        ImGui::EndTable();
    }

    // Fillet section
    if (f.filletItemId != 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Fillet");
        if (f.filletIconUrl && f.filletIconUrl[0]) {
            ImGui::SameLine();
            Texture_t* ft = CastAway::IconManager::GetIcon(f.filletItemId);
            if (ft && ft->Resource) {
                ImGui::Image((ImTextureID)(uintptr_t)ft->Resource, {16.f, 16.f});
            } else {
                CastAway::IconManager::RequestIcon(f.filletItemId, f.filletIconUrl);
                ImGui::Dummy({16.f, 16.f});
            }
            if (f.filletName) {
                ImGui::SameLine();
                ImGui::TextUnformatted(f.filletName);
            }
        } else if (f.filletName) {
            ImGui::SameLine();
            ImGui::TextUnformatted(f.filletName);
        }

        const PriceInfo* fp = g_Prices.Get(f.filletItemId);
        if (fp && fp->fetched) {
            if (fp->tradeable) {
                ImGui::TextDisabled("  Buy:  %s", FormatCoins(fp->buy_price).c_str());
                ImGui::TextDisabled("  Sell: %s", FormatCoins(fp->sell_price).c_str());
            } else {
                ImGui::TextDisabled("  Not tradeable");
            }
        } else {
            ImGui::TextDisabled("  Loading prices...");
        }
    }

    // Fish TP prices
    if (f.itemId != 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Trading Post");
        const PriceInfo* pp = g_Prices.Get(f.itemId);
        if (pp && pp->fetched) {
            if (pp->tradeable) {
                ImGui::TextDisabled("  Buy:  %s", FormatCoins(pp->buy_price).c_str());
                ImGui::TextDisabled("  Sell: %s", FormatCoins(pp->sell_price).c_str());
            } else {
                ImGui::TextDisabled("  Not tradeable");
            }
        } else {
            ImGui::TextDisabled("  Loading prices...");
        }
    }

    // Wiki button
    if (f.wikiSlug && f.wikiSlug[0]) {
        ImGui::Spacing();
        if (ImGui::SmallButton("Open Wiki")) {
            std::string url = std::string("https://wiki.guildwars2.com/wiki/") + f.wikiSlug;
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }
}

// ---------------------------------------------------------------------------
// Sort helper
// ---------------------------------------------------------------------------
static void RebuildSortedFishIndices(const ImGuiTableSortSpecs* sortSpecs) {
    if (!sortSpecs || sortSpecs->SpecsCount == 0) {
        std::iota(g_SortedFishIndices.begin(), g_SortedFishIndices.end(), 0);
        return;
    }
    const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
    bool asc = (spec.SortDirection == ImGuiSortDirection_Ascending);

    // Column indices: 0=icon, 1=star, 2=Fish, 3=Map, 4=Bait, 5=Time, 6=Water, 7=Caught
    std::sort(g_SortedFishIndices.begin(), g_SortedFishIndices.end(),
        [&](int a, int b) {
            const Fish& fa = FISH_TABLE[a];
            const Fish& fb = FISH_TABLE[b];
            int cmp = 0;
            switch (spec.ColumnIndex) {
                case 2: cmp = strcmp(fa.name,               fb.name);               break;
                case 3: cmp = strcmp(fa.map  ? fa.map  : "", fb.map  ? fb.map  : ""); break;
                case 4: cmp = (int)fa.bait  - (int)fb.bait;                          break;
                case 5: cmp = (int)fa.time  - (int)fb.time;                          break;
                case 6: cmp = (int)fa.water - (int)fb.water;                         break;
                default: cmp = strcmp(fa.name, fb.name);                              break;
            }
            return asc ? (cmp < 0) : (cmp > 0);
        });
}

// ---------------------------------------------------------------------------
// AddonRender
// ---------------------------------------------------------------------------
void AddonRender() {
    // --- MumbleLink ---
    int   mumbleMapId = 0;
    float game_x = 0.f, game_z = 0.f;

    MumbleLinkedMem* mumble =
        reinterpret_cast<MumbleLinkedMem*>(APIDefs->DataLink_Get(DL_MUMBLE_LINK));
    if (mumble && mumble->uiTick > 0) {
        const GW2Context* ctx =
            reinterpret_cast<const GW2Context*>(mumble->context);
        mumbleMapId = (int)ctx->mapId;
        game_x = mumble->fAvatarPosition[0];
        game_z = mumble->fAvatarPosition[2];

        if (mumbleMapId != 0) {
            for (int i = 0; i < HOLE_COUNT; i++) {
                if ((int)HOLE_TABLE[i].mapId == mumbleMapId && HOLE_TABLE[i].map) {
                    g_CurrentMapName = HOLE_TABLE[i].map;
                    break;
                }
            }
        }
    }

    // --- Per-frame work ---
    g_AchTracker.FlushPendingQuery();
    g_MapPanel.ProcessReadyQueue();
    CastAway::IconManager::Tick();

    UpdateHoleDwellTimers(mumbleMapId, game_x, game_z);
    CheckTimeWindowNotifications();
    CheckNearbyHoles(mumbleMapId, game_x, game_z);

    RenderOverlay();
    RenderToasts();

    if (!g_WindowVisible) return;

    // --- Main window ---
    ImGui::SetNextWindowSize({800.f, 560.f}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Cast Away##MainWindow", &g_WindowVisible,
                      ImGuiWindowFlags_NoScrollbar)) {
        ImGui::End();
        return;
    }

    float contentWidth = ImGui::GetContentRegionAvail().x;
    RenderDayNightBar(contentWidth);
    ImGui::Spacing();

    // Tab navigation from Favourites → Database
    static int forceTab = -1;
    if (g_SwitchToDatabase) {
        forceTab = 0;
        g_SwitchToDatabase = false;
    }

    if (ImGui::BeginTabBar("##CastAwayTabs")) {

        // ===== DATABASE TAB =====
        ImGuiTabItemFlags dbFlags = (forceTab == 0) ? ImGuiTabItemFlags_SetSelected : 0;
        if (forceTab == 0) forceTab = -1;
        if (ImGui::BeginTabItem("Database", nullptr, dbFlags)) {

            // Filter bar
            ImGui::SetNextItemWidth(140.f);
            ImGui::InputText("##Search", g_SearchBuf, sizeof(g_SearchBuf));
            ImGui::SameLine();

            ImGui::SetNextItemWidth(110.f);
            if (ImGui::BeginCombo("##Bait",
                    g_FilterBait == 0 ? "All Bait" : BAIT_NAMES[g_FilterBait])) {
                if (ImGui::Selectable("All Bait", g_FilterBait == 0)) g_FilterBait = 0;
                for (int i = 1; i < BAIT_COUNT; i++)
                    if (ImGui::Selectable(BAIT_NAMES[i], g_FilterBait == i)) g_FilterBait = i;
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            static const char* timeOpts[] = {"All Times","Dawn","Day","Dusk","Night"};
            const char* timeLbl = g_ShowCurrentOnly ? "Now"
                                : timeOpts[g_FilterTime < 0 ? 0 : g_FilterTime];
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::BeginCombo("##Time", timeLbl)) {
                if (ImGui::Selectable("All Times", !g_ShowCurrentOnly && g_FilterTime == 0)) {
                    g_ShowCurrentOnly = false; g_FilterTime = 0;
                }
                for (int i = 1; i <= 4; i++) {
                    if (ImGui::Selectable(timeOpts[i], !g_ShowCurrentOnly && g_FilterTime == i)) {
                        g_ShowCurrentOnly = false; g_FilterTime = i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            // Water type filter: combo index offset by +1 to account for "All Water" at index 0
            static const char* waterOpts[] = {"All Water","Freshwater","Saltwater","Special"};
            int waterComboIdx = g_FilterWater + 1; // -1 → 0, 0 → 1, 1 → 2, 2 → 3
            ImGui::SetNextItemWidth(95.f);
            if (ImGui::BeginCombo("##Water", waterOpts[waterComboIdx])) {
                for (int i = 0; i < 4; i++)
                    if (ImGui::Selectable(waterOpts[i], waterComboIdx == i))
                        g_FilterWater = i - 1;
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            if (ImGui::SmallButton("Now")) {
                g_ShowCurrentOnly = true;
                g_FilterMap = g_CurrentMapName;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                g_SearchBuf[0]    = '\0';
                g_FilterBait      = 0;
                g_FilterTime      = 0;
                g_FilterWater     = -1;
                g_ShowCurrentOnly = false;
                g_MissingOnly     = false;
                g_FilterMap.clear();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Missing only", &g_MissingOnly);

            ImGui::Spacing();

            // Split layout
            float detailsW = (g_SelectedFish >= 0) ? 260.f : 0.f;
            float tableW   = contentWidth - detailsW - (detailsW > 0 ? 8.f : 0.f);
            float tableH   = ImGui::GetContentRegionAvail().y - 4.f;

            ImGuiTableFlags tflags =
                ImGuiTableFlags_Sortable      |
                ImGuiTableFlags_RowBg         |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_ScrollY       |
                ImGuiTableFlags_SizingFixedFit;

            if (ImGui::BeginTable("##FishTable", 8, tflags, {tableW, tableH})) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("##Icon",  ImGuiTableColumnFlags_NoSort |
                                                   ImGuiTableColumnFlags_WidthFixed, 22.f);
                ImGui::TableSetupColumn("##Star",  ImGuiTableColumnFlags_NoSort |
                                                   ImGuiTableColumnFlags_WidthFixed, 20.f);
                ImGui::TableSetupColumn("Fish",    ImGuiTableColumnFlags_DefaultSort |
                                                   ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Map",     ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Bait",    ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Time",    ImGuiTableColumnFlags_WidthFixed,  54.f);
                ImGui::TableSetupColumn("Water",   ImGuiTableColumnFlags_WidthFixed,  72.f);
                ImGui::TableSetupColumn("Caught",  ImGuiTableColumnFlags_NoSort |
                                                   ImGuiTableColumnFlags_WidthFixed,  54.f);
                ImGui::TableHeadersRow();

                // Apply sort
                if (ImGuiTableSortSpecs* ss = ImGui::TableGetSortSpecs()) {
                    if (ss->SpecsDirty) {
                        RebuildSortedFishIndices(ss);
                        ss->SpecsDirty = false;
                    }
                }

                for (int idx : g_SortedFishIndices) {
                    if (!FishMatchesFilter(idx)) continue;
                    const Fish& f = FISH_TABLE[idx];

                    ImGui::TableNextRow();

                    // Icon
                    ImGui::TableSetColumnIndex(0);
                    if (f.itemId != 0) {
                        Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
                        if (tex && tex->Resource) {
                            ImGui::Image((ImTextureID)(uintptr_t)tex->Resource, {18.f, 18.f});
                        } else {
                            CastAway::IconManager::RequestIcon(f.itemId,
                                                               f.iconUrl ? f.iconUrl : "");
                            ImGui::Dummy({18.f, 18.f});
                        }
                    } else {
                        ImGui::Dummy({18.f, 18.f});
                    }

                    // Star
                    ImGui::TableSetColumnIndex(1);
                    {
                        bool fav = IsFavourite(f.name);
                        char starId[64];
                        snprintf(starId, sizeof(starId),
                                 "%s##star%d",
                                 fav ? "\xe2\x98\x85" : "\xe2\x98\x86", idx);
                        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.f,0.9f,0.2f,0.2f});
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {1.f,0.9f,0.2f,0.4f});
                        if (fav) ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.1f,1.f});
                        if (ImGui::SmallButton(starId)) ToggleFavourite(f.name);
                        if (fav) ImGui::PopStyleColor();
                        ImGui::PopStyleColor(3);
                    }

                    // Fish name (selectable spanning remaining columns)
                    ImGui::TableSetColumnIndex(2);
                    {
                        char selId[128];
                        snprintf(selId, sizeof(selId), "%s##row%d", f.name, idx);
                        if (ImGui::Selectable(selId, g_SelectedFish == idx,
                                ImGuiSelectableFlags_SpanAllColumns)) {
                            g_SelectedFish = (g_SelectedFish == idx) ? -1 : idx;
                        }
                    }

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(f.map ? f.map : "-");

                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(BAIT_NAMES[(int)f.bait]);

                    ImGui::TableSetColumnIndex(5);
                    ImGui::TextUnformatted(TimeOfDayName(f.time));

                    ImGui::TableSetColumnIndex(6);
                    ImGui::TextUnformatted(WaterTypeName(f.water));

                    ImGui::TableSetColumnIndex(7);
                    if (g_AchTracker.hoarded) {
                        if (g_AchTracker.IsCaught(idx)) {
                            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "\xe2\x9c\x93"); // ✓
                        } else {
                            ImGui::TextDisabled("\xe2\x9c\x97"); // ✗
                        }
                    } else {
                        ImGui::TextDisabled("\xe2\x80\x94"); // —
                    }
                }
                ImGui::EndTable();
            }

            // Details panel
            if (g_SelectedFish >= 0 && detailsW > 0) {
                ImGui::SameLine();
                if (ImGui::BeginChild("##FishDetailsPanel", {detailsW, tableH},
                                      true)) {
                    RenderFishDetails(g_SelectedFish);
                }
                ImGui::EndChild();
            }

            ImGui::EndTabItem();
        }

        // ===== FAVOURITES TAB =====
        if (ImGui::BeginTabItem("Favourites")) {
            float favH = ImGui::GetContentRegionAvail().y - 4.f;
            ImGuiTableFlags fflags =
                ImGuiTableFlags_RowBg         |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_ScrollY       |
                ImGuiTableFlags_SizingStretchProp;

            if (ImGui::BeginTable("##FavTable", 5, fflags, {-1.f, favH})) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("##Star", ImGuiTableColumnFlags_WidthFixed, 20.f);
                ImGui::TableSetupColumn("Fish",   ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Map",    ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Bait",   ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Time",   ImGuiTableColumnFlags_WidthFixed, 54.f);
                ImGui::TableHeadersRow();

                for (int i = 0; i < FISH_COUNT; i++) {
                    const Fish& f = FISH_TABLE[i];
                    if (!IsFavourite(f.name)) continue;

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    {
                        char starId[64];
                        snprintf(starId, sizeof(starId), "\xe2\x98\x85##fstar%d", i);
                        ImGui::PushStyleColor(ImGuiCol_Button, {0,0,0,0});
                        ImGui::PushStyleColor(ImGuiCol_Text,   {1.f,0.85f,0.1f,1.f});
                        if (ImGui::SmallButton(starId)) ToggleFavourite(f.name);
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::TableSetColumnIndex(1);
                    {
                        char selId[128];
                        snprintf(selId, sizeof(selId), "%s##frow%d", f.name, i);
                        if (ImGui::Selectable(selId, g_SelectedFish == i,
                                ImGuiSelectableFlags_SpanAllColumns)) {
                            g_SelectedFish = (g_SelectedFish == i) ? -1 : i;
                            if (g_SelectedFish >= 0) g_SwitchToDatabase = true;
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(f.map ? f.map : "-");

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(BAIT_NAMES[(int)f.bait]);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(TimeOfDayName(f.time));
                }
                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        // ===== MAP TAB =====
        if (ImGui::BeginTabItem("Map")) {
            if (g_SelectedFish < 0) {
                ImGui::TextDisabled(
                    "Select a fish in the Database tab to highlight its fishing holes.");
                ImGui::Spacing();
            }
            g_MapPanel.Render(g_SelectedFish, mumbleMapId, game_x, game_z);
            ImGui::EndTabItem();
        }

        // ===== ACHIEVEMENTS TAB =====
        if (ImGui::BeginTabItem("Achievements")) {
            if (!g_AchTracker.hoarded) {
                ImGui::TextWrapped("Requires the Hoard & Seek addon to be installed and configured.");
                ImGui::TextDisabled("Install Hoard & Seek, then restart GW2.");
            } else {
                for (int ci = 0; ci < COLLECTION_COUNT; ++ci) {
                    const FishingCollection& col = COLLECTION_TABLE[ci];
                    if (col.achievementId == 0) continue;
                    const CollectionState* st = g_AchTracker.GetCollection(col.achievementId);
                    if (!st) continue;

                    // Achievement icon (20x20)
                    if (col.iconUrl && col.iconUrl[0]) {
                        uint32_t iconKey = col.achievementId + 9000000u;
                        Texture_t* ach_icon = CastAway::IconManager::GetIcon(iconKey);
                        if (ach_icon && ach_icon->Resource)
                            ImGui::Image(ach_icon->Resource, {20, 20});
                        else {
                            CastAway::IconManager::RequestIcon(iconKey, col.iconUrl);
                            ImGui::Dummy({20, 20});
                        }
                        ImGui::SameLine();
                    }

                    char header[128];
                    snprintf(header, sizeof(header), "%s  %u/%u##col%d",
                             col.name, st->caughtCount, st->totalFish, ci);
                    bool open = ImGui::CollapsingHeader(header);

                    // Progress bar — render after CollapsingHeader
                    float frac = st->totalFish > 0 ? (float)st->caughtCount / st->totalFish : 0.f;
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 160.f);
                    ImGui::SetNextItemWidth(150.f);
                    ImGui::ProgressBar(frac, {150.f, ImGui::GetTextLineHeight()});

                    if (!open) continue;

                    ImGui::Indent();
                    for (int fi = 0; fi < FISH_COUNT; ++fi) {
                        const Fish& f = FISH_TABLE[fi];
                        if (f.achievementId != col.achievementId) continue;

                        bool caught = st->done || (st->bitsKnown && fi < FISH_COUNT &&
                                      f.bitIndex < 64 && st->caughtBits[f.bitIndex]);

                        // Fish icon (16x16)
                        if (f.itemId != 0) {
                            Texture_t* fish_icon = CastAway::IconManager::GetIcon(f.itemId);
                            if (fish_icon && fish_icon->Resource)
                                ImGui::Image(fish_icon->Resource, {16, 16});
                            else {
                                CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
                                ImGui::Dummy({16, 16});
                            }
                            ImGui::SameLine();
                        }

                        if (caught) {
                            ImGui::TextDisabled("\xe2\x9c\x93  %s", f.name);
                        } else {
                            // Clickable — select fish and switch to Database tab
                            char label[128];
                            snprintf(label, sizeof(label), "\xe2\x9c\x97  %s  (%s / %s)##achfish%d",
                                     f.name, BAIT_NAMES[(int)f.bait], TimeOfDayName(f.time), fi);
                            if (ImGui::Selectable(label, false)) {
                                g_SelectedFish     = fi;
                                g_SwitchToDatabase = true;
                            }
                        }
                    }
                    ImGui::Unindent();
                }
                // Bait shopping list collapsible section (Task 15)
                ImGui::Spacing();
                if (ImGui::CollapsingHeader("Bait Shopping List")) {
                    RenderBaitShoppingList();
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// AddonOptions
// ---------------------------------------------------------------------------
void AddonOptions() {
    ImGui::Text("Cast Away v%d.%d", V_MAJOR, V_MINOR);
    ImGui::Separator();

    ImGui::Checkbox("Show time overlay", &g_OverlayVisible);

    bool prevQA = g_ShowQAIcon;
    ImGui::Checkbox("Show quick access icon", &g_ShowQAIcon);
    if (g_ShowQAIcon != prevQA) {
        if (g_ShowQAIcon)
            APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER,
                                     "KB_CAST_AWAY_TOGGLE", "Cast Away");
        else
            APIDefs->QuickAccess_Remove(QA_ID);
    }

    ImGui::SliderInt("Notify before window (seconds)", &g_NotifyLeadSeconds, 30, 600);
    ImGui::Spacing();
    ImGui::TextDisabled("Toggle: Ctrl+Shift+F");
}

// ---------------------------------------------------------------------------
// GetAddonDef
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    AddonDef.Signature        = 0xCA57A000;
    AddonDef.APIVersion       = NEXUS_API_VERSION;
    AddonDef.Name             = "Cast Away";
    AddonDef.Version.Major    = V_MAJOR;
    AddonDef.Version.Minor    = V_MINOR;
    AddonDef.Version.Build    = V_BUILD;
    AddonDef.Version.Revision = V_REVISION;
    AddonDef.Author           = "PieOrCake.7635";
    AddonDef.Description      =
        "Fishing companion: searchable fish database with time-of-day, bait info, and interactive map.";
    AddonDef.Load             = AddonLoad;
    AddonDef.Unload           = AddonUnload;
    AddonDef.Flags            = AF_None;
    AddonDef.Provider         = UP_GitHub;
    AddonDef.UpdateLink       = "https://github.com/PieOrCake/cast_away";
    return &AddonDef;
}

// ---------------------------------------------------------------------------
// AddonLoad
// ---------------------------------------------------------------------------
void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;

    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void*(*)(size_t, void*))APIDefs->ImguiMalloc,
        (void(*)(void*, void*))APIDefs->ImguiFree);

    CastAway::IconManager::Initialize(APIDefs);
    g_AchTracker.Init(APIDefs);

    std::string dataDir = std::string(APIDefs->Paths_GetAddonDirectory("cast_away"));
    std::filesystem::create_directories(dataDir);
    g_MapPanel.Init(APIDefs, dataDir);

    g_SortedFishIndices.resize((size_t)FISH_COUNT);
    std::iota(g_SortedFishIndices.begin(), g_SortedFishIndices.end(), 0);

    APIDefs->GUI_Register(RT_Render, AddonRender);
    APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);

    APIDefs->InputBinds_RegisterWithString(
        "KB_CAST_AWAY_TOGGLE",
        [](const char*, bool rel) { if (!rel) g_WindowVisible = !g_WindowVisible; },
        "CTRL+SHIFT+F");

    APIDefs->GUI_RegisterCloseOnEscape("Cast Away##MainWindow", &g_WindowVisible);

    APIDefs->Textures_LoadFromMemory(TEX_ICON,       (void*)ICON_NORMAL, ICON_NORMAL_size, nullptr);
    APIDefs->Textures_LoadFromMemory(TEX_ICON_HOVER, (void*)ICON_HOVER,  ICON_HOVER_size,  nullptr);

    if (g_ShowQAIcon)
        APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER,
                                 "KB_CAST_AWAY_TOGGLE", "Cast Away");

    LoadSettings();
}

// ---------------------------------------------------------------------------
// AddonUnload
// ---------------------------------------------------------------------------
void AddonUnload() {
    SaveSettings();

    APIDefs->QuickAccess_Remove(QA_ID);
    APIDefs->GUI_DeregisterCloseOnEscape("Cast Away##MainWindow");
    APIDefs->InputBinds_Deregister("KB_CAST_AWAY_TOGGLE");
    APIDefs->GUI_Deregister(AddonOptions);
    APIDefs->GUI_Deregister(AddonRender);

    g_AchTracker.Shutdown();
    g_MapPanel.Shutdown();
    CastAway::IconManager::Shutdown();

    APIDefs = nullptr;
}
