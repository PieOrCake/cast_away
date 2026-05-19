#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include "nexus/Nexus.h"
#include "imgui.h"
#include "TileCache.h"
#include "FishData.h"

struct MapBoundsData {
    float map_min_x, map_min_y, map_max_x, map_max_y;
    float cont_min_x, cont_min_y, cont_max_x, cont_max_y;
    bool  valid = false;
};

class MapPanel {
public:
    void Init(AddonAPI_t* api, const std::string& dataDir);
    void Shutdown();
    void ProcessReadyQueue();

    // selectedFishIdx: index into FISH_TABLE (-1 = none selected)
    // mumbleMapId: from MumbleLink context.mapId (0 = unknown)
    // game_x/game_z: MumbleLink fAvatarPosition[0]/[2] in metres
    void Render(int selectedFishIdx, int mumbleMapId, float game_x, float game_z);

    void FetchAllMapBounds(); // called in background thread from Init

private:
    ImVec2 ContToScreen(ImVec2 wp, ImVec2 ws, float cx, float cy) const;
    bool   MumbleToCont(uint32_t mapId, float gx, float gz, float& cx, float& cy) const;
    void   RenderTiles(ImDrawList* dl, ImVec2 wp, ImVec2 ws);
    void   RenderHoles(ImDrawList* dl, ImVec2 wp, ImVec2 ws, int selectedFishIdx,
                       float game_x, float game_z);
    void   RenderPlayerDot(ImDrawList* dl, ImVec2 wp, ImVec2 ws, float gx, float gz);
    static void DrawMarchingAnts(ImDrawList* dl, ImVec2 a, ImVec2 b, float time, ImU32 col);

    bool FetchBoundsForMap(uint32_t mapId);
    void LoadBoundsCache();
    void SaveBoundsCache();

    AddonAPI_t* m_api = nullptr;
    std::string m_dataDir;
    TileCache   m_tiles;

    float    m_zoom   = 0.15f;
    float    m_orig_x = 16384.f;
    float    m_orig_y = 16384.f;
    uint32_t m_lastMapId  = 0;
    bool     m_prefetched = false;

    // Background thread for FetchAllMapBounds
    std::thread m_fetchThread;
    bool        m_shutdownFetch = false;

    mutable std::mutex m_boundsMu;
    std::unordered_map<uint32_t, MapBoundsData> m_bounds;
};
