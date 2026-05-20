#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <thread>
#include "nexus/Nexus.h"
#include "imgui.h"
#include "TileCache.h"
#include "FishData.h"

struct MapBoundsData {
    float map_min_x, map_min_y, map_max_x, map_max_y;
    float cont_min_x, cont_min_y, cont_max_x, cont_max_y;
    int   default_floor = 1;
    bool  valid = false;
};

struct MapWaypoint {
    std::string name;
    std::string chatLink;
    float cont_x, cont_y;
};

struct MapLabel {
    std::string name;
    float cont_x, cont_y;
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

    // Navigate the map view to a specific map by ID (used by notification Open Map buttons)
    void NavigateToMap(uint32_t mapId);

    void FetchAllMapBounds(); // called in background thread from Init

private:
    ImVec2 ContToScreen(ImVec2 wp, ImVec2 ws, float cx, float cy) const;
    bool   MumbleToCont(uint32_t mapId, float gx, float gz, float& cx, float& cy) const;
    void   RenderTiles(ImDrawList* dl, ImVec2 wp, ImVec2 ws);
    void   RenderHoles(ImDrawList* dl, ImVec2 wp, ImVec2 ws, int selectedFishIdx,
                       float game_x, float game_z);
    void   RenderPlayerDot(ImDrawList* dl, ImVec2 wp, ImVec2 ws, int mumbleMapId, float gx, float gz);
    static void DrawMarchingAnts(ImDrawList* dl, ImVec2 a, ImVec2 b, float time, ImU32 col);

    bool FetchBoundsForMap(uint32_t mapId);
    void FetchAllWaypoints();
    void RenderWaypoints(ImDrawList* dl, ImVec2 wp, ImVec2 ws);
    void RenderLabels(ImDrawList* dl, ImVec2 wp, ImVec2 ws);
    void LoadBoundsCache();
    void SaveBoundsCache();
    void LoadWaypointsCache();
    void SaveWaypointsCache();

    AddonAPI_t* m_api = nullptr;
    std::string m_dataDir;
    TileCache   m_tiles;

    float    m_zoom   = 0.15f;
    float    m_orig_x = 49064.f; // Lion's Arch (centre of Tyria)
    float    m_orig_y = 30911.f;
    uint32_t m_lastMapId        = 0;
    int      m_lastSelectedFish = -2; // -2 = uninitialised
    bool     m_prefetched       = false;
    int      m_lastPrefetchZ    = -1; // last zoom level tiles were prefetched for
    uint32_t m_lastWaypointLogMapId = 0xFFFFFFFFu; // last map id we logged waypoint count for

    // Background thread for FetchAllMapBounds
    std::thread m_fetchThread;
    bool        m_shutdownFetch = false;

    mutable std::mutex m_boundsMu;
    std::unordered_map<uint32_t, MapBoundsData>              m_bounds;
    std::unordered_map<uint32_t, std::vector<MapWaypoint>>   m_waypoints;
    std::vector<MapLabel>                                    m_regionLabels;
    std::vector<MapLabel>                                    m_mapLabels;
    std::vector<MapLabel>                                    m_sectorLabels;
    bool                                                     m_waypointsFetched = false;
};
