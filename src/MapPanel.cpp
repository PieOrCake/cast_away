#include "MapPanel.h"
#include "IconManager.h"
#include "HoleLocations.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static void CopyToClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (h) {
        memcpy(GlobalLock(h), text.c_str(), text.size() + 1);
        GlobalUnlock(h);
        SetClipboardData(CF_TEXT, h);
    }
    CloseClipboard();
}

static std::string HttpGet(const std::string& url) {
    HINTERNET hInet = InternetOpenA("CastAway/1.0", INTERNET_OPEN_TYPE_PRECONFIG,
                                    nullptr, nullptr, 0);
    if (!hInet) return {};

    HINTERNET hConn = InternetOpenUrlA(hInet, url.c_str(), nullptr, 0,
                                       INTERNET_FLAG_SECURE |
                                       INTERNET_FLAG_RELOAD |
                                       INTERNET_FLAG_NO_CACHE_WRITE, 0);
    std::string result;
    if (hConn) {
        char buf[4096];
        DWORD read = 0;
        while (InternetReadFile(hConn, buf, sizeof(buf), &read) && read > 0) {
            result.append(buf, read);
            read = 0;
        }
        InternetCloseHandle(hConn);
    }
    InternetCloseHandle(hInet);
    return result;
}

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void MapPanel::Init(AddonAPI_t* api, const std::string& dataDir) {
    m_api     = api;
    m_dataDir = dataDir;
    m_tiles.Init(api, dataDir);  // TileCache appends "/map_tiles" itself
    LoadBoundsCache();
    LoadWaypointsCache();
    m_fetchThread = std::thread(&MapPanel::FetchAllMapBounds, this);
}

void MapPanel::Shutdown() {
    m_shutdownFetch = true;
    if (m_fetchThread.joinable()) m_fetchThread.join();
    m_tiles.Shutdown();
}

void MapPanel::ProcessReadyQueue() {
    m_tiles.ProcessReadyQueue();
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

ImVec2 MapPanel::ContToScreen(ImVec2 wp, ImVec2 ws, float cx, float cy) const {
    // continent 0..32768 → panel pixels
    float sx = wp.x + (cx - m_orig_x) * m_zoom + ws.x * 0.5f;
    float sy = wp.y + (cy - m_orig_y) * m_zoom + ws.y * 0.5f;
    return ImVec2(sx, sy);
}

bool MapPanel::MumbleToCont(uint32_t mapId, float gx, float gz,
                             float& cx, float& cy) const {
    std::lock_guard<std::mutex> lk(m_boundsMu);
    auto it = m_bounds.find(mapId);
    if (it == m_bounds.end() || !it->second.valid) return false;
    const MapBoundsData& b = it->second;

    float ix = gx * 39.3701f;
    float iy = gz * 39.3701f;
    cx = (ix - b.map_min_x) / (b.map_max_x - b.map_min_x)
           * (b.cont_max_x - b.cont_min_x) + b.cont_min_x;
    cy = (b.map_max_y - iy) / (b.map_max_y - b.map_min_y)
           * (b.cont_max_y - b.cont_min_y) + b.cont_min_y;
    return true;
}

// ---------------------------------------------------------------------------
// Tile rendering
// ---------------------------------------------------------------------------

void MapPanel::RenderTiles(ImDrawList* dl, ImVec2 wp, ImVec2 ws) {
    // Choose tile zoom: highest detail level where tiles appear at or below native size.
    // Native zoom for level z: 256px / (32768/2^z cu) = 2^z/128 px/cu
    // → z = ceil(log2(128 * m_zoom)), clamped to [4,7]
    int chosenZ = std::max(4, std::min(7, (int)ceilf(log2f(128.f * m_zoom))));

    float span    = 32768.f / (float)(1 << chosenZ);
    float tilePx  = span * m_zoom;

    // Visible continent rect
    float visMinX = m_orig_x - ws.x * 0.5f / m_zoom;
    float visMinY = m_orig_y - ws.y * 0.5f / m_zoom;
    float visMaxX = m_orig_x + ws.x * 0.5f / m_zoom;
    float visMaxY = m_orig_y + ws.y * 0.5f / m_zoom;

    // GW2 continent 1 is 81920 × 114688 continent units — NOT a power-of-2 square
    int maxTileX = (int)ceilf(81920.f  / span) - 1;
    int maxTileY = (int)ceilf(114688.f / span) - 1;

    int txMin = std::max((int)floorf(visMinX / span), 0);
    int tyMin = std::max((int)floorf(visMinY / span), 0);
    int txMax = std::min((int)floorf(visMaxX / span), maxTileX);
    int tyMax = std::min((int)floorf(visMaxY / span), maxTileY);

    // Prefetch map region tiles when zoom level changes or map/fish changed.
    if (chosenZ != m_lastPrefetchZ || !m_prefetched) {
        m_lastPrefetchZ = chosenZ;
        m_prefetched    = true;

        float pfMinX = visMinX - (visMaxX - visMinX) * 0.5f;
        float pfMinY = visMinY - (visMaxY - visMinY) * 0.5f;
        float pfMaxX = visMaxX + (visMaxX - visMinX) * 0.5f;
        float pfMaxY = visMaxY + (visMaxY - visMinY) * 0.5f;
        bool usedBounds = false;

        {
            std::lock_guard<std::mutex> lk(m_boundsMu);
            auto it = m_bounds.find(m_lastMapId);
            if (it != m_bounds.end() && it->second.valid) {
                pfMinX = it->second.cont_min_x;
                pfMinY = it->second.cont_min_y;
                pfMaxX = it->second.cont_max_x;
                pfMaxY = it->second.cont_max_y;
                usedBounds = true;
            }
        }

        char msg[256];
        snprintf(msg, sizeof(msg),
            "[Map] PrefetchView z=%d map=%u region=[%.0f,%.0f,%.0f,%.0f] source=%s",
            chosenZ, m_lastMapId, pfMinX, pfMinY, pfMaxX, pfMaxY,
            usedBounds ? "map_bounds" : "visible_area_fallback");
        if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);

        m_tiles.PrefetchView(chosenZ, pfMinX, pfMinY, pfMaxX, pfMaxY);
    }

    for (int ty = tyMin; ty <= tyMax; ++ty) {
        for (int tx = txMin; tx <= txMax; ++tx) {
            float tcx = tx * span;
            float tcy = ty * span;
            ImVec2 tl = ContToScreen(wp, ws, tcx,        tcy);
            ImVec2 br = ContToScreen(wp, ws, tcx + span, tcy + span);

            Texture_t* tex = m_tiles.GetTile(chosenZ, tx, ty);
            if (tex && tex->Resource) {
                dl->AddImage((ImTextureID)tex->Resource, tl, br);
            } else {
                dl->AddRectFilled(tl, br, IM_COL32(30, 30, 40, 255));
                dl->AddRect(tl, br, IM_COL32(50, 50, 60, 255));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Marching ants
// ---------------------------------------------------------------------------

void MapPanel::DrawMarchingAnts(ImDrawList* dl, ImVec2 a, ImVec2 b,
                                 float t, ImU32 col) {
    const float dashLen = 8.f;
    const float gapLen  = 4.f;
    const float period  = dashLen + gapLen;
    const float speed   = 30.f;

    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.f) return;
    float nx = dx / len, ny = dy / len;

    float offset = fmodf(t * speed, period);

    float pos = -offset;
    while (pos < len) {
        float dStart = std::max(pos, 0.f);
        float dEnd   = std::min(pos + dashLen, len);
        if (dEnd > dStart) {
            ImVec2 p0(a.x + nx * dStart, a.y + ny * dStart);
            ImVec2 p1(a.x + nx * dEnd,   a.y + ny * dEnd);
            dl->AddLine(p0, p1, col, 1.5f);
        }
        pos += period;
    }
}

// ---------------------------------------------------------------------------
// Hole rendering
// ---------------------------------------------------------------------------

void MapPanel::RenderHoles(ImDrawList* dl, ImVec2 wp, ImVec2 ws,
                            int selectedFishIdx, float game_x, float game_z) {
    (void)game_x; (void)game_z;

    HoleWater filterType = HoleWater::Any;
    if (selectedFishIdx >= 0 && selectedFishIdx < FISH_COUNT)
        filterType = FISH_TABLE[selectedFishIdx].holeType;

    // Bounds for the currently shown map are required to convert game coords.
    MapBoundsData b;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        auto it = m_bounds.find(m_lastMapId);
        if (it == m_bounds.end() || !it->second.valid) return;
        b = it->second;
    }

    // Pre-compute affine factors once per frame.
    const float ix_scale = (b.cont_max_x - b.cont_min_x) / (b.map_max_x - b.map_min_x);
    const float iy_scale = (b.cont_max_y - b.cont_min_y) / (b.map_max_y - b.map_min_y);

    for (int i = 0; i < HOLE_LOCATION_COUNT; ++i) {
        const HoleLocation& h = HOLE_LOCATION_TABLE[i];
        if (h.mapId != m_lastMapId) continue;
        if (filterType != HoleWater::Any && h.water != filterType) continue;

        // game metres -> map inches -> continent coords
        float ix = h.game_x * 39.3701f;
        float iy = h.game_z * 39.3701f;
        float cx = (ix - b.map_min_x) * ix_scale + b.cont_min_x;
        float cy = (b.map_max_y - iy) * iy_scale + b.cont_min_y;

        ImVec2 hPos = ContToScreen(wp, ws, cx, cy);
        if (hPos.x < wp.x - 6.f || hPos.x > wp.x + ws.x + 6.f ||
            hPos.y < wp.y - 6.f || hPos.y > wp.y + ws.y + 6.f) continue;

        dl->AddCircleFilled(hPos, 4.f, IM_COL32(80, 160, 240, 230));
        dl->AddCircle(hPos, 4.f, IM_COL32(255, 255, 255, 200), 0, 1.2f);

        // Hover tooltip showing the hole's water type
        ImVec2 btnMin(hPos.x - 4.f, hPos.y - 4.f);
        ImGui::SetCursorScreenPos(btnMin);
        char btnId[24];
        snprintf(btnId, sizeof(btnId), "##hl%d", i);
        ImGui::InvisibleButton(btnId, ImVec2(8.f, 8.f));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", HoleWaterName(h.water));
            ImGui::EndTooltip();
        }
    }
}

// ---------------------------------------------------------------------------
// Player dot
// ---------------------------------------------------------------------------

void MapPanel::RenderPlayerDot(ImDrawList* dl, ImVec2 wp, ImVec2 ws,
                                int mumbleMapId, float gx, float gz) {
    if (gx == 0.f && gz == 0.f) return;
    if (mumbleMapId <= 0 || mumbleMapId >= 100000) return;
    // Only draw the player dot when the displayed map matches the player's actual map.
    if ((uint32_t)mumbleMapId != m_lastMapId) return;

    float cx, cy;
    if (!MumbleToCont((uint32_t)mumbleMapId, gx, gz, cx, cy)) return;

    ImVec2 pos = ContToScreen(wp, ws, cx, cy);
    if (pos.x < wp.x || pos.x > wp.x + ws.x ||
        pos.y < wp.y || pos.y > wp.y + ws.y) return;

    dl->AddCircleFilled(pos, 5.f, IM_COL32(255, 210, 50, 255));
    dl->AddCircle(pos, 5.f, IM_COL32(0, 0, 0, 200), 0, 1.5f);
}

// ---------------------------------------------------------------------------
// Public Navigation
// ---------------------------------------------------------------------------

void MapPanel::NavigateToMap(uint32_t mapId) {
    if (mapId == 0 || mapId == m_lastMapId) return;
    m_lastMapId      = mapId;
    m_prefetched     = false;
    m_lastPrefetchZ  = -1;
    m_lastSelectedFish = -2;  // reset so Render won't immediately override
}

// ---------------------------------------------------------------------------
// Main Render
// ---------------------------------------------------------------------------

void MapPanel::Render(int selectedFishIdx, int mumbleMapId, float game_x, float game_z) {
    ImVec2 wp = ImGui::GetCursorScreenPos();
    ImVec2 ws = ImGui::GetContentRegionAvail();
    if (ws.x < 10.f || ws.y < 10.f) return;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(20, 25, 35, 255));

    // Map ID change — try to centre on player.
    // Reject garbage values (real GW2 map IDs are well under 100k).
    if (mumbleMapId > 0 && mumbleMapId < 100000 && (uint32_t)mumbleMapId != m_lastMapId) {
        m_lastMapId   = (uint32_t)mumbleMapId;
        m_prefetched  = false;
        float cx, cy;
        if (game_x != 0.f && MumbleToCont(m_lastMapId, game_x, game_z, cx, cy)) {
            m_orig_x = cx;
            m_orig_y = cy;
        }
    }

    // Re-centre whenever the selected fish changes
    if (selectedFishIdx != m_lastSelectedFish) {
        m_lastSelectedFish = selectedFishIdx;
        if (selectedFishIdx >= 0 && selectedFishIdx < FISH_COUNT) {
            // Match the fish's region (Fish.map) against HOLE_TABLE.map. Using fishIds
            // would only catch the 8 fish each HOLE_TABLE entry happens to list, missing
            // the other 6–13 fish per region.
            const Fish& fish = FISH_TABLE[selectedFishIdx];
            for (int i = 0; i < HOLE_COUNT; ++i) {
                const FishingHole& hole = HOLE_TABLE[i];
                if (!fish.map || !hole.map || strcmp(fish.map, hole.map) != 0) continue;

                m_lastMapId      = hole.mapId;
                m_prefetched     = false;
                m_lastPrefetchZ  = -1;
                {
                    std::lock_guard<std::mutex> lk(m_boundsMu);
                    auto it = m_bounds.find(hole.mapId);
                    if (it != m_bounds.end() && it->second.valid) {
                        const MapBoundsData& b = it->second;
                        if (hole.game_x != 0.f || hole.game_z != 0.f) {
                            float ix = hole.game_x * 39.3701f;
                            float iy = hole.game_z * 39.3701f;
                            m_orig_x = (ix - b.map_min_x) / (b.map_max_x - b.map_min_x)
                                           * (b.cont_max_x - b.cont_min_x) + b.cont_min_x;
                            m_orig_y = (b.map_max_y - iy) / (b.map_max_y - b.map_min_y)
                                           * (b.cont_max_y - b.cont_min_y) + b.cont_min_y;
                            char msg[192];
                            snprintf(msg, sizeof(msg),
                                "[Map] Centre on hole '%s' map %u: game(%.1f,%.1f) -> cont(%.0f,%.0f)",
                                hole.name, hole.mapId, hole.game_x, hole.game_z, m_orig_x, m_orig_y);
                            if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);
                        } else {
                            m_orig_x = (b.cont_min_x + b.cont_max_x) * 0.5f;
                            m_orig_y = (b.cont_min_y + b.cont_max_y) * 0.5f;
                            char msg[192];
                            snprintf(msg, sizeof(msg),
                                "[Map] Centre on map region %u (no hole coords): cont(%.0f,%.0f)",
                                hole.mapId, m_orig_x, m_orig_y);
                            if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);
                        }
                    } else {
                        char msg[128];
                        snprintf(msg, sizeof(msg),
                            "[Map] No bounds for map %u — cannot centre (bounds not yet fetched?)", hole.mapId);
                        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
                    }
                }
                break;
            }
        }
    }

    RenderTiles(dl, wp, ws);
    RenderHoles(dl, wp, ws, selectedFishIdx, game_x, game_z);
    RenderWaypoints(dl, wp, ws);
    RenderPlayerDot(dl, wp, ws, mumbleMapId, game_x, game_z);
    RenderLabels(dl, wp, ws);

    // Mouse input — invisible button covering the whole panel
    ImGui::SetCursorScreenPos(wp);
    ImGui::InvisibleButton("##mapcanvas", ws, ImGuiButtonFlags_MouseButtonLeft);
    bool hovered = ImGui::IsItemHovered();
    bool active  = ImGui::IsItemActive();

    // Pan
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        m_orig_x -= delta.x / m_zoom;
        m_orig_y -= delta.y / m_zoom;
    }

    // Zoom toward cursor
    if (hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.f) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            // Continent coord under mouse before zoom
            float cx = m_orig_x + (mouse.x - wp.x - ws.x * 0.5f) / m_zoom;
            float cy = m_orig_y + (mouse.y - wp.y - ws.y * 0.5f) / m_zoom;
            float factor = (wheel > 0.f) ? 1.15f : (1.f / 1.15f);
            // Min: whole continent (~81920 cu) visible. Max: z=7 native (no blurry upscale).
            m_zoom = std::max(0.008f, std::min(m_zoom * factor, 1.0f));
            // Re-anchor origin so the point under mouse stays fixed
            m_orig_x = cx - (mouse.x - wp.x - ws.x * 0.5f) / m_zoom;
            m_orig_y = cy - (mouse.y - wp.y - ws.y * 0.5f) / m_zoom;
        }
    }

    // Clamp origin to GW2 continent 1 bounds: 81920 × 114688 continent units
    m_orig_x = std::max(0.f, std::min(m_orig_x, 81920.f));
    m_orig_y = std::max(0.f, std::min(m_orig_y, 114688.f));
}

// ---------------------------------------------------------------------------
// Bounds fetch / cache
// ---------------------------------------------------------------------------

bool MapPanel::FetchBoundsForMap(uint32_t mapId) {
    char msg[256];
    std::string url = "https://api.guildwars2.com/v2/maps/" + std::to_string(mapId);
    snprintf(msg, sizeof(msg), "[Map] FetchBounds: GET %s", url.c_str());
    if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);

    std::string body = HttpGet(url);
    if (body.empty()) {
        snprintf(msg, sizeof(msg), "[Map] FetchBounds: empty response for map %u", mapId);
        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
        return false;
    }

    snprintf(msg, sizeof(msg), "[Map] FetchBounds: got %zu bytes for map %u", body.size(), mapId);
    if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);

    try {
        auto j = json::parse(body);
        auto cr = j.at("continent_rect");
        auto mr = j.at("map_rect");

        MapBoundsData b;
        b.cont_min_x = cr[0][0].get<float>();
        b.cont_min_y = cr[0][1].get<float>();
        b.cont_max_x = cr[1][0].get<float>();
        b.cont_max_y = cr[1][1].get<float>();
        b.map_min_x  = mr[0][0].get<float>();
        b.map_min_y  = mr[0][1].get<float>();
        b.map_max_x     = mr[1][0].get<float>();
        b.map_max_y     = mr[1][1].get<float>();
        b.default_floor = j.value("default_floor", 1);
        b.valid         = true;

        snprintf(msg, sizeof(msg),
            "[Map] FetchBounds map %u: cont=[%.0f,%.0f,%.0f,%.0f] map=[%.0f,%.0f,%.0f,%.0f]",
            mapId, b.cont_min_x, b.cont_min_y, b.cont_max_x, b.cont_max_y,
            b.map_min_x, b.map_min_y, b.map_max_x, b.map_max_y);
        if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);

        std::lock_guard<std::mutex> lk(m_boundsMu);
        m_bounds[mapId] = b;
        return true;
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "[Map] FetchBounds parse error map %u: %s", mapId, e.what());
        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
        return false;
    } catch (...) {
        snprintf(msg, sizeof(msg), "[Map] FetchBounds unknown parse error map %u", mapId);
        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
        return false;
    }
}

void MapPanel::FetchAllMapBounds() {
    std::unordered_set<uint32_t> ids;
    for (int i = 0; i < HOLE_LOCATION_COUNT; ++i)
        ids.insert(HOLE_LOCATION_TABLE[i].mapId);

    char msg[128];
    snprintf(msg, sizeof(msg), "[Map] FetchAllMapBounds: %zu unique map IDs", ids.size());
    if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);

    int fetched = 0, skipped = 0;
    for (uint32_t id : ids) {
        if (m_shutdownFetch) break;
        {
            std::lock_guard<std::mutex> lk(m_boundsMu);
            if (m_bounds.count(id) && m_bounds[id].valid) { ++skipped; continue; }
        }
        if (FetchBoundsForMap(id)) ++fetched;
    }
    snprintf(msg, sizeof(msg), "[Map] FetchAllMapBounds done: %d fetched, %d skipped (cached)", fetched, skipped);
    if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);

    bool needWaypoints;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        needWaypoints = !m_waypointsFetched;
    }
    if (needWaypoints && !m_shutdownFetch)
        FetchAllWaypoints();

    SaveBoundsCache();
}

// ---------------------------------------------------------------------------
// Waypoint fetch from GW2 continents API
// ---------------------------------------------------------------------------

// Fetch all waypoints for all maps in one shot, mirroring AEM's approach:
// GET /v2/continents/1/floors/{floor}/regions?ids=all  (floors 1 and 49)
// Returns an array of regions, each containing maps, each containing points_of_interest.
// Hardcoded whitelist of "real" open-world map IDs on continent 1. Mirrors AEM —
// the API also returns story instances, festival copies, raids, guild halls etc.
// that have POIs and sectors but overlay the same continent coords; filtering by
// this whitelist is the only reliable way to exclude them.
static const std::unordered_set<uint32_t> WORLD_MAP_IDS = {
    // Core Tyria + cities
    15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,34,35,39,
    50,51,53,54,62,65,73,91,139,218,326,335,873,
    // Season 1 / Season 2
    922,988,1015,
    // HoT
    1041,1043,1045,1052,
    // LW Season 3
    1165,1175,1178,1185,1195,1203,
    // PoF
    1210,1211,1226,1228,1248,
    // LW Season 4
    1263,1271,1288,1301,1310,1317,
    // IBS
    1330,1343,1370,1371,
    // EoD
    1422,1428,1438,1442,1452,1490,
    // SotO
    1509,1510,1517,1526,
    // Janthir Wilds
    1550,1554,1574,1575,
    // VoE / Castora
    1593,1595,
};

void MapPanel::FetchAllWaypoints() {
    char msg[256];
    std::unordered_map<uint32_t, std::vector<MapWaypoint>> result;
    std::vector<MapLabel> regionLabels;
    std::vector<MapLabel> mapLabels;
    std::vector<MapLabel> sectorLabels;
    std::unordered_set<std::string> seenRegions;
    std::unordered_set<uint32_t> seenMapLabels;
    std::unordered_set<uint32_t> seenSectorMaps;

    for (int floor : {1, 49}) {
        if (m_shutdownFetch) return;
        std::string url = "https://api.guildwars2.com/v2/continents/1/floors/"
                        + std::to_string(floor) + "/regions?ids=all";
        snprintf(msg, sizeof(msg), "[Map] FetchAllWaypoints floor %d: GET %s", floor, url.c_str());
        if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);

        std::string body = HttpGet(url);
        if (body.empty()) {
            snprintf(msg, sizeof(msg), "[Map] FetchAllWaypoints floor %d: empty response", floor);
            if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
            continue;
        }

        try {
            auto regions = json::parse(body);
            int totalWp = 0;
            int totalMaps = 0;
            for (auto& region : regions) {
                // Region label — same filter: skip empty/(0,0) coords.
                std::string regionName = region.value("name", "");
                if (!regionName.empty() && seenRegions.insert(regionName).second
                    && region.contains("label_coord") && region["label_coord"].size() >= 2) {
                    float lx = region["label_coord"][0].get<float>();
                    float ly = region["label_coord"][1].get<float>();
                    if (lx != 0.f || ly != 0.f) {
                        MapLabel lbl;
                        lbl.name   = std::move(regionName);
                        lbl.cont_x = lx;
                        lbl.cont_y = ly;
                        regionLabels.push_back(std::move(lbl));
                    }
                }

                if (!region.contains("maps")) continue;
                for (auto& [mapKey, mapData] : region["maps"].items()) {
                    uint32_t mapId = (uint32_t)std::stoul(mapKey);
                    ++totalMaps;

                    // Only process labels/sectors for known open-world maps (AEM's approach).
                    // Story instances, festival copies, raids, guild halls etc. all have POIs
                    // and sectors at overlapping coords and would otherwise pile up.
                    const bool isWorldMap = WORLD_MAP_IDS.count(mapId) > 0;

                    // Map label
                    if (isWorldMap && seenMapLabels.insert(mapId).second
                        && mapData.contains("label_coord")
                        && mapData["label_coord"].size() >= 2) {
                        float lx = mapData["label_coord"][0].get<float>();
                        float ly = mapData["label_coord"][1].get<float>();
                        std::string lname = mapData.value("name", "");
                        if (!lname.empty() && (lx != 0.f || ly != 0.f)) {
                            MapLabel lbl;
                            lbl.name = std::move(lname);
                            lbl.cont_x = lx;
                            lbl.cont_y = ly;
                            mapLabels.push_back(std::move(lbl));
                        }
                    }

                    // Sectors (sub-area names like "Daigo Ward", "Shinota Shore")
                    if (isWorldMap && seenSectorMaps.insert(mapId).second
                        && mapData.contains("sectors")) {
                        for (auto& [sectKey, sect] : mapData["sectors"].items()) {
                            if (!sect.contains("coord") || sect["coord"].size() < 2) continue;
                            float sx = sect["coord"][0].get<float>();
                            float sy = sect["coord"][1].get<float>();
                            if (sx == 0.f && sy == 0.f) continue;
                            std::string sname = sect.value("name", "");
                            if (sname.empty()) continue;
                            MapLabel lbl;
                            lbl.name   = std::move(sname);
                            lbl.cont_x = sx;
                            lbl.cont_y = sy;
                            sectorLabels.push_back(std::move(lbl));
                        }
                    }

                    if (!mapData.contains("points_of_interest")) continue;
                    // If this map already has waypoints from a prior floor, skip
                    // to avoid duplicates (a map may appear on multiple floors).
                    if (result.count(mapId)) continue;
                    for (auto& [poiKey, poi] : mapData["points_of_interest"].items()) {
                        if (poi.value("type", "") != "waypoint") continue;
                        if (!poi.contains("coord") || poi["coord"].size() < 2) continue;
                        MapWaypoint w;
                        w.name     = poi.value("name", "Waypoint");
                        w.chatLink = poi.value("chat_link", "");
                        w.cont_x   = poi["coord"][0].get<float>();
                        w.cont_y   = poi["coord"][1].get<float>();
                        result[mapId].push_back(std::move(w));
                        ++totalWp;
                    }
                }
            }
            snprintf(msg, sizeof(msg), "[Map] FetchAllWaypoints floor %d: %d waypoints across %d maps", floor, totalWp, totalMaps);
            if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);

            // Diagnostic: log the waypoint count for each fishing-hole map specifically.
            std::unordered_set<uint32_t> seen;
            for (int hi = 0; hi < HOLE_COUNT; ++hi) {
                uint32_t hmap = HOLE_TABLE[hi].mapId;
                if (hmap == 0 || !seen.insert(hmap).second) continue;
                auto it = result.find(hmap);
                int n = (it == result.end()) ? -1 : (int)it->second.size();
                snprintf(msg, sizeof(msg),
                    "[Map]   floor %d hole-map %u: %s%d waypoints",
                    floor, hmap, n < 0 ? "(not found in this floor) " : "", n < 0 ? 0 : n);
                if (m_api) m_api->Log(LOGL_DEBUG, "CastAway", msg);
            }
        } catch (const std::exception& e) {
            snprintf(msg, sizeof(msg), "[Map] FetchAllWaypoints floor %d parse error: %s", floor, e.what());
            if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
        } catch (...) {
            if (m_api) m_api->Log(LOGL_WARNING, "CastAway", "[Map] FetchAllWaypoints: unknown parse error");
        }
    }

    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        m_waypoints        = std::move(result);
        m_regionLabels     = std::move(regionLabels);
        m_mapLabels        = std::move(mapLabels);
        m_sectorLabels     = std::move(sectorLabels);
        m_waypointsFetched = true;
    }
    SaveWaypointsCache();
}

// ---------------------------------------------------------------------------
// Waypoint rendering
// ---------------------------------------------------------------------------

static const uint32_t WP_ICON_ID  = 157353;
static const char*    WP_ICON_URL = "https://render.guildwars2.com/file/"
                                    "32633AF8ADEA696A1EF56D3AE32D617B10D3AC57/157353.png";

void MapPanel::RenderWaypoints(ImDrawList* dl, ImVec2 wp, ImVec2 ws) {
    Texture_t* wpTex = CastAway::IconManager::GetIcon(WP_ICON_ID);
    if (!wpTex)
        CastAway::IconManager::RequestIcon(WP_ICON_ID, WP_ICON_URL);

    std::vector<MapWaypoint> waypoints;
    bool fetched;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        fetched = m_waypointsFetched;
        auto it = m_waypoints.find(m_lastMapId);
        if (it != m_waypoints.end()) waypoints = it->second;
    }

    // Log waypoint count once whenever the map changes — helps diagnose missing data.
    if (m_lastMapId != m_lastWaypointLogMapId) {
        m_lastWaypointLogMapId = m_lastMapId;
        char msg[160];
        snprintf(msg, sizeof(msg),
            "[Map] RenderWaypoints: map=%u count=%zu fetched=%d",
            m_lastMapId, waypoints.size(), fetched ? 1 : 0);
        if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);
    }

    if (waypoints.empty()) return;

    const float half = 12.f;
    for (int i = 0; i < (int)waypoints.size(); ++i) {
        const MapWaypoint& w = waypoints[i];
        ImVec2 pos = ContToScreen(wp, ws, w.cont_x, w.cont_y);

        if (pos.x < wp.x - half || pos.x > wp.x + ws.x + half ||
            pos.y < wp.y - half || pos.y > wp.y + ws.y + half) continue;

        ImVec2 tl(pos.x - half, pos.y - half);
        ImVec2 br(pos.x + half, pos.y + half);

        if (wpTex && wpTex->Resource) {
            dl->AddImage((ImTextureID)(uintptr_t)wpTex->Resource, tl, br);
        } else {
            // Fallback: gold diamond until icon loads
            dl->AddQuadFilled(
                ImVec2(pos.x,        pos.y - half),
                ImVec2(pos.x + half, pos.y),
                ImVec2(pos.x,        pos.y + half),
                ImVec2(pos.x - half, pos.y),
                IM_COL32(255, 200, 50, 220));
        }

        ImGui::SetCursorScreenPos(tl);
        char btnId[32];
        snprintf(btnId, sizeof(btnId), "##apiWP%d", i);
        ImGui::InvisibleButton(btnId, ImVec2(half * 2.f, half * 2.f));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", w.name.c_str());
            if (!w.chatLink.empty())
                ImGui::TextDisabled("%s — click to copy", w.chatLink.c_str());
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemClicked() && !w.chatLink.empty())
            CopyToClipboard(w.chatLink);
    }
}

// ---------------------------------------------------------------------------
// Region / map name labels (AEM-style)
// ---------------------------------------------------------------------------

void MapPanel::RenderLabels(ImDrawList* dl, ImVec2 wp, ImVec2 ws) {
    // Three label tiers (matches in-game map):
    //   regions  ("Cantha", "Kryta")       — shown when zoomed far out
    //   maps     ("Seitung Province")       — shown at mid zoom
    //   sectors  ("Daigo Ward")             — shown when zoomed onto a map
    const bool showRegions = m_zoom <  0.06f;
    const bool showMaps    = m_zoom >= 0.03f && m_zoom < 0.20f;
    const bool showSectors = m_zoom >= 0.10f;

    auto drawLabel = [&](const MapLabel& l, float fontScale, ImU32 col) {
        ImVec2 pos = ContToScreen(wp, ws, l.cont_x, l.cont_y);
        if (pos.x < wp.x - 80.f || pos.x > wp.x + ws.x + 80.f ||
            pos.y < wp.y - 20.f || pos.y > wp.y + ws.y + 20.f) return;
        float fontSize = ImGui::GetFontSize() * fontScale;
        ImVec2 tsz = ImGui::CalcTextSize(l.name.c_str());
        tsz.x *= fontScale; tsz.y *= fontScale;
        ImVec2 tp(pos.x - tsz.x * 0.5f, pos.y - tsz.y * 0.5f);
        // Soft shadow for readability over varying tile colours (no opaque box)
        ImU32 shadow = IM_COL32(0, 0, 0, 200);
        dl->AddText(nullptr, fontSize, ImVec2(tp.x + 1, tp.y + 1), shadow, l.name.c_str());
        dl->AddText(nullptr, fontSize, ImVec2(tp.x - 1, tp.y - 1), shadow, l.name.c_str());
        dl->AddText(nullptr, fontSize, ImVec2(tp.x + 1, tp.y - 1), shadow, l.name.c_str());
        dl->AddText(nullptr, fontSize, ImVec2(tp.x - 1, tp.y + 1), shadow, l.name.c_str());
        dl->AddText(nullptr, fontSize, tp, col, l.name.c_str());
    };

    std::vector<MapLabel> regions, maps, sectors;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        if (showRegions) regions = m_regionLabels;
        if (showMaps)    maps    = m_mapLabels;
        if (showSectors) sectors = m_sectorLabels;
    }

    if (showRegions)
        for (auto& r : regions) drawLabel(r, 1.4f, IM_COL32(255, 230, 150, 235));
    if (showMaps)
        for (auto& m : maps)    drawLabel(m, 1.1f, IM_COL32(245, 245, 250, 230));
    if (showSectors)
        for (auto& s : sectors) drawLabel(s, 0.95f, IM_COL32(220, 220, 230, 220));
}

void MapPanel::LoadBoundsCache() {
    std::string path = m_dataDir + "/map_bounds.json";
    std::ifstream f(path);
    if (!f.is_open()) return;

    try {
        json j;
        f >> j;
        std::lock_guard<std::mutex> lk(m_boundsMu);
        int count = 0;
        for (auto& [key, val] : j.items()) {
            uint32_t mapId = (uint32_t)std::stoul(key);
            MapBoundsData b;
            b.cont_min_x = val["cont_min_x"].get<float>();
            b.cont_min_y = val["cont_min_y"].get<float>();
            b.cont_max_x = val["cont_max_x"].get<float>();
            b.cont_max_y = val["cont_max_y"].get<float>();
            b.map_min_x  = val["map_min_x"].get<float>();
            b.map_min_y  = val["map_min_y"].get<float>();
            b.map_max_x     = val["map_max_x"].get<float>();
            b.map_max_y     = val["map_max_y"].get<float>();
            b.default_floor = val.value("default_floor", 1);
            b.valid         = true;
            m_bounds[mapId] = b;
            ++count;
        }
        char msg[128];
        snprintf(msg, sizeof(msg), "[Map] LoadBoundsCache: loaded %d map bounds from %s", count, path.c_str());
        if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);
    } catch (const std::exception& e) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[Map] LoadBoundsCache parse error: %s", e.what());
        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", msg);
    } catch (...) {
        if (m_api) m_api->Log(LOGL_WARNING, "CastAway", "[Map] LoadBoundsCache unknown error");
    }
}

void MapPanel::SaveBoundsCache() {
    std::string path = m_dataDir + "/map_bounds.json";
    json j;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        for (auto& [mapId, b] : m_bounds) {
            if (!b.valid) continue;
            json entry;
            entry["cont_min_x"] = b.cont_min_x;
            entry["cont_min_y"] = b.cont_min_y;
            entry["cont_max_x"] = b.cont_max_x;
            entry["cont_max_y"] = b.cont_max_y;
            entry["map_min_x"]  = b.map_min_x;
            entry["map_min_y"]  = b.map_min_y;
            entry["map_max_x"]      = b.map_max_x;
            entry["map_max_y"]      = b.map_max_y;
            entry["default_floor"]  = b.default_floor;
            j[std::to_string(mapId)] = entry;
        }
    }
    try {
        std::filesystem::create_directories(m_dataDir);
        std::ofstream f(path);
        f << j.dump(2);
    } catch (...) {}
}

void MapPanel::LoadWaypointsCache() {
    std::string path = m_dataDir + "/map_waypoints.json";
    std::ifstream f(path);
    if (!f.is_open()) return;
    try {
        json j;
        f >> j;
        std::lock_guard<std::mutex> lk(m_boundsMu);
        int total = 0;

        // Waypoints (object keyed by mapId, or {"waypoints": ..., "regions": ..., "maps": ...})
        auto wpRoot = j.contains("waypoints") ? j["waypoints"] : j;
        for (auto& [key, arr] : wpRoot.items()) {
            uint32_t mapId = (uint32_t)std::stoul(key);
            std::vector<MapWaypoint> wps;
            for (auto& wj : arr) {
                MapWaypoint w;
                w.name     = wj.value("name", "Waypoint");
                w.chatLink = wj.value("chat_link", "");
                w.cont_x   = wj.value("x", 0.f);
                w.cont_y   = wj.value("y", 0.f);
                wps.push_back(std::move(w));
                ++total;
            }
            m_waypoints[mapId] = std::move(wps);
        }
        if (j.contains("regions")) {
            for (auto& lj : j["regions"]) {
                MapLabel l;
                l.name   = lj.value("name", "");
                l.cont_x = lj.value("x", 0.f);
                l.cont_y = lj.value("y", 0.f);
                if (!l.name.empty()) m_regionLabels.push_back(std::move(l));
            }
        }
        if (j.contains("maps")) {
            for (auto& lj : j["maps"]) {
                MapLabel l;
                l.name   = lj.value("name", "");
                l.cont_x = lj.value("x", 0.f);
                l.cont_y = lj.value("y", 0.f);
                if (!l.name.empty()) m_mapLabels.push_back(std::move(l));
            }
        }
        if (j.contains("sectors")) {
            for (auto& lj : j["sectors"]) {
                MapLabel l;
                l.name   = lj.value("name", "");
                l.cont_x = lj.value("x", 0.f);
                l.cont_y = lj.value("y", 0.f);
                if (!l.name.empty()) m_sectorLabels.push_back(std::move(l));
            }
        }
        if (total > 0) m_waypointsFetched = true;
        char msg[160];
        snprintf(msg, sizeof(msg),
            "[Map] LoadWaypointsCache: %d waypoints across %zu maps, %zu regions/%zu map labels",
            total, m_waypoints.size(), m_regionLabels.size(), m_mapLabels.size());
        if (m_api) m_api->Log(LOGL_INFO, "CastAway", msg);
    } catch (...) {}
}

void MapPanel::SaveWaypointsCache() {
    std::string path = m_dataDir + "/map_waypoints.json";
    json j;
    {
        std::lock_guard<std::mutex> lk(m_boundsMu);
        json wpRoot = json::object();
        for (auto& [mapId, wps] : m_waypoints) {
            json arr = json::array();
            for (auto& w : wps)
                arr.push_back({{"name", w.name}, {"chat_link", w.chatLink},
                               {"x", w.cont_x}, {"y", w.cont_y}});
            wpRoot[std::to_string(mapId)] = arr;
        }
        j["waypoints"] = wpRoot;

        json regs = json::array();
        for (auto& l : m_regionLabels)
            regs.push_back({{"name", l.name}, {"x", l.cont_x}, {"y", l.cont_y}});
        j["regions"] = regs;

        json maps = json::array();
        for (auto& l : m_mapLabels)
            maps.push_back({{"name", l.name}, {"x", l.cont_x}, {"y", l.cont_y}});
        j["maps"] = maps;

        json sects = json::array();
        for (auto& l : m_sectorLabels)
            sects.push_back({{"name", l.name}, {"x", l.cont_x}, {"y", l.cont_y}});
        j["sectors"] = sects;
    }
    try {
        std::filesystem::create_directories(m_dataDir);
        std::ofstream f(path);
        f << j.dump(2);
    } catch (...) {}
}
