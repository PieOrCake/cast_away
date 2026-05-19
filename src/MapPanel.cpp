#include "MapPanel.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cmath>
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
    // Choose zoom level: smallest z where tile pixels >= 200
    float tileSpan = 32768.f;   // continent units per tile at z=0
    int chosenZ = 4;
    for (int z = 4; z <= 7; ++z) {
        float span  = 32768.f / (float)(1 << z);
        float pxWid = span * m_zoom;
        chosenZ = z;
        if (pxWid >= 200.f) break;
    }

    float span    = 32768.f / (float)(1 << chosenZ);
    float tilePx  = span * m_zoom;

    // Visible continent rect
    float visMinX = m_orig_x - ws.x * 0.5f / m_zoom;
    float visMinY = m_orig_y - ws.y * 0.5f / m_zoom;
    float visMaxX = m_orig_x + ws.x * 0.5f / m_zoom;
    float visMaxY = m_orig_y + ws.y * 0.5f / m_zoom;

    int txMin = (int)floorf(visMinX / span);
    int tyMin = (int)floorf(visMinY / span);
    int txMax = (int)floorf(visMaxX / span);
    int tyMax = (int)floorf(visMaxY / span);
    int maxTile = (1 << chosenZ) - 1;
    txMin = std::max(txMin, 0);  tyMin = std::max(tyMin, 0);
    txMax = std::min(txMax, maxTile); tyMax = std::min(tyMax, maxTile);

    for (int ty = tyMin; ty <= tyMax; ++ty) {
        for (int tx = txMin; tx <= txMax; ++tx) {
            // Top-left continent coord of this tile
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
    float t = (float)ImGui::GetTime();

    for (int i = 0; i < HOLE_COUNT; ++i) {
        const FishingHole& hole = HOLE_TABLE[i];

        if (selectedFishIdx >= 0) {
            // Only show holes that contain the selected fish
            bool found = false;
            for (int fi = 0; fi < hole.fishCount; ++fi) {
                if (hole.fishIds[fi] == (uint16_t)selectedFishIdx) { found = true; break; }
            }
            if (!found) continue;
        } else {
            // No fish selected — show all holes on the current map only
            if (hole.mapId != m_lastMapId) continue;
        }

        // Convert hole game coords to continent
        // HOLE_TABLE stores game_x/game_z directly in metres
        float cx, cy;
        {
            std::lock_guard<std::mutex> lk(m_boundsMu);
            auto it = m_bounds.find(hole.mapId);
            if (it == m_bounds.end() || !it->second.valid) continue;
            const MapBoundsData& b = it->second;
            float ix = hole.game_x * 39.3701f;
            float iy = hole.game_z * 39.3701f;
            cx = (ix - b.map_min_x) / (b.map_max_x - b.map_min_x)
                   * (b.cont_max_x - b.cont_min_x) + b.cont_min_x;
            cy = (b.map_max_y - iy) / (b.map_max_y - b.map_min_y)
                   * (b.cont_max_y - b.cont_min_y) + b.cont_min_y;
        }

        ImVec2 hPos = ContToScreen(wp, ws, cx, cy);

        // Only draw if on-screen
        bool onScreen = hPos.x >= wp.x && hPos.x <= wp.x + ws.x &&
                        hPos.y >= wp.y && hPos.y <= wp.y + ws.y;

        // Waypoint
        const Waypoint& wpt = WAYPOINT_TABLE[hole.waypointIdx];
        bool hasWaypoint = (wpt.cont_x != 0.f);
        ImVec2 wPos;
        if (hasWaypoint) {
            wPos = ContToScreen(wp, ws, wpt.cont_x, wpt.cont_y);
            bool wOnScreen = wPos.x >= wp.x && wPos.x <= wp.x + ws.x &&
                             wPos.y >= wp.y && wPos.y <= wp.y + ws.y;
            if (wOnScreen || onScreen) {
                // Marching ants line waypoint → hole
                DrawMarchingAnts(dl, wPos, hPos, t, IM_COL32(255, 220, 80, 200));
            }
        }

        if (onScreen) {
            // Blue hole circle
            dl->AddCircleFilled(hPos, 7.f, IM_COL32(60, 100, 200, 220));
            dl->AddCircle(hPos, 7.f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
        }

        if (hasWaypoint) {
            bool wOnScreen = wPos.x >= wp.x && wPos.x <= wp.x + ws.x &&
                             wPos.y >= wp.y && wPos.y <= wp.y + ws.y;
            if (wOnScreen) {
                // Gold diamond for waypoint
                float r = 6.f;
                dl->AddQuadFilled(
                    ImVec2(wPos.x,     wPos.y - r),
                    ImVec2(wPos.x + r, wPos.y),
                    ImVec2(wPos.x,     wPos.y + r),
                    ImVec2(wPos.x - r, wPos.y),
                    IM_COL32(255, 200, 50, 220));
                dl->AddQuad(
                    ImVec2(wPos.x,     wPos.y - r),
                    ImVec2(wPos.x + r, wPos.y),
                    ImVec2(wPos.x,     wPos.y + r),
                    ImVec2(wPos.x - r, wPos.y),
                    IM_COL32(255, 255, 200, 200), 1.5f);

                // Invisible button over waypoint
                ImVec2 btnMin(wPos.x - r, wPos.y - r);
                ImGui::SetCursorScreenPos(btnMin);
                std::string btnId = std::string("##wpt") + std::to_string(i);
                ImGui::InvisibleButton(btnId.c_str(), ImVec2(r * 2.f, r * 2.f));
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", wpt.name);
                    ImGui::TextDisabled("%s", wpt.chatLink);
                    ImGui::EndTooltip();
                }
                if (ImGui::IsItemClicked()) {
                    CopyToClipboard(std::string(wpt.chatLink));
                }
            }
        }

        if (onScreen) {
            // Invisible button over hole
            ImVec2 btnMin(hPos.x - 7.f, hPos.y - 7.f);
            ImGui::SetCursorScreenPos(btnMin);
            std::string btnId = std::string("##hole") + std::to_string(i);
            ImGui::InvisibleButton(btnId.c_str(), ImVec2(14.f, 14.f));
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", hole.name);
                ImGui::EndTooltip();
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Player dot
// ---------------------------------------------------------------------------

void MapPanel::RenderPlayerDot(ImDrawList* dl, ImVec2 wp, ImVec2 ws,
                                float gx, float gz) {
    if (gx == 0.f && gz == 0.f) return;
    if (m_lastMapId == 0) return;

    float cx, cy;
    if (!MumbleToCont(m_lastMapId, gx, gz, cx, cy)) return;

    ImVec2 pos = ContToScreen(wp, ws, cx, cy);
    if (pos.x < wp.x || pos.x > wp.x + ws.x ||
        pos.y < wp.y || pos.y > wp.y + ws.y) return;

    dl->AddCircleFilled(pos, 5.f, IM_COL32(255, 210, 50, 255));
    dl->AddCircle(pos, 5.f, IM_COL32(0, 0, 0, 200), 0, 1.5f);
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

    // Map ID change — try to centre on player
    if (mumbleMapId != 0 && (uint32_t)mumbleMapId != m_lastMapId) {
        m_lastMapId   = (uint32_t)mumbleMapId;
        m_prefetched  = false;
        float cx, cy;
        if (game_x != 0.f && MumbleToCont(m_lastMapId, game_x, game_z, cx, cy)) {
            m_orig_x = cx;
            m_orig_y = cy;
        }
    }

    // Auto-jump to fish's map when a fish is selected that's on a different map
    if (selectedFishIdx >= 0) {
        for (int i = 0; i < HOLE_COUNT; ++i) {
            const FishingHole& hole = HOLE_TABLE[i];
            bool found = false;
            for (int fi = 0; fi < hole.fishCount; ++fi) {
                if (hole.fishIds[fi] == (uint16_t)selectedFishIdx) { found = true; break; }
            }
            if (!found) continue;

            if (hole.mapId != m_lastMapId) {
                m_lastMapId = hole.mapId;
                // Try to centre on this hole
                std::lock_guard<std::mutex> lk(m_boundsMu);
                auto it = m_bounds.find(hole.mapId);
                if (it != m_bounds.end() && it->second.valid) {
                    const MapBoundsData& b = it->second;
                    float ix = hole.game_x * 39.3701f;
                    float iy = hole.game_z * 39.3701f;
                    m_orig_x = (ix - b.map_min_x) / (b.map_max_x - b.map_min_x)
                                   * (b.cont_max_x - b.cont_min_x) + b.cont_min_x;
                    m_orig_y = (b.map_max_y - iy) / (b.map_max_y - b.map_min_y)
                                   * (b.cont_max_y - b.cont_min_y) + b.cont_min_y;
                }
            }
            break;
        }
    }

    RenderTiles(dl, wp, ws);
    RenderHoles(dl, wp, ws, selectedFishIdx, game_x, game_z);
    RenderPlayerDot(dl, wp, ws, game_x, game_z);

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
            m_zoom = std::max(0.02f, std::min(m_zoom * factor, 4.f));
            // Re-anchor origin so the point under mouse stays fixed
            m_orig_x = cx - (mouse.x - wp.x - ws.x * 0.5f) / m_zoom;
            m_orig_y = cy - (mouse.y - wp.y - ws.y * 0.5f) / m_zoom;
        }
    }

    // Clamp origin to continent bounds
    m_orig_x = std::max(0.f, std::min(m_orig_x, 32768.f));
    m_orig_y = std::max(0.f, std::min(m_orig_y, 32768.f));
}

// ---------------------------------------------------------------------------
// Bounds fetch / cache
// ---------------------------------------------------------------------------

bool MapPanel::FetchBoundsForMap(uint32_t mapId) {
    std::string url = "https://api.guildwars2.com/v2/maps/" + std::to_string(mapId);
    std::string body = HttpGet(url);
    if (body.empty()) return false;

    try {
        auto j = json::parse(body);
        // map_rect: [[min_x,min_y],[max_x,max_y]] in inches
        // continent_rect: same in continent coords
        auto cr = j.at("continent_rect");
        auto mr = j.at("map_rect");

        MapBoundsData b;
        b.cont_min_x = cr[0][0].get<float>();
        b.cont_min_y = cr[0][1].get<float>();
        b.cont_max_x = cr[1][0].get<float>();
        b.cont_max_y = cr[1][1].get<float>();
        b.map_min_x  = mr[0][0].get<float>();
        b.map_min_y  = mr[0][1].get<float>();
        b.map_max_x  = mr[1][0].get<float>();
        b.map_max_y  = mr[1][1].get<float>();
        b.valid      = true;

        std::lock_guard<std::mutex> lk(m_boundsMu);
        m_bounds[mapId] = b;
        return true;
    } catch (...) {
        return false;
    }
}

void MapPanel::FetchAllMapBounds() {
    // Collect unique map IDs from HOLE_TABLE
    std::unordered_set<uint32_t> ids;
    for (int i = 0; i < HOLE_COUNT; ++i)
        ids.insert(HOLE_TABLE[i].mapId);

    for (uint32_t id : ids) {
        if (m_shutdownFetch) break;
        {
            std::lock_guard<std::mutex> lk(m_boundsMu);
            if (m_bounds.count(id) && m_bounds[id].valid) continue;
        }
        FetchBoundsForMap(id);
    }
    SaveBoundsCache();
}

void MapPanel::LoadBoundsCache() {
    std::string path = m_dataDir + "/map_bounds.json";
    std::ifstream f(path);
    if (!f.is_open()) return;

    try {
        json j;
        f >> j;
        std::lock_guard<std::mutex> lk(m_boundsMu);
        for (auto& [key, val] : j.items()) {
            uint32_t mapId = (uint32_t)std::stoul(key);
            MapBoundsData b;
            b.cont_min_x = val["cont_min_x"].get<float>();
            b.cont_min_y = val["cont_min_y"].get<float>();
            b.cont_max_x = val["cont_max_x"].get<float>();
            b.cont_max_y = val["cont_max_y"].get<float>();
            b.map_min_x  = val["map_min_x"].get<float>();
            b.map_min_y  = val["map_min_y"].get<float>();
            b.map_max_x  = val["map_max_x"].get<float>();
            b.map_max_y  = val["map_max_y"].get<float>();
            b.valid      = true;
            m_bounds[mapId] = b;
        }
    } catch (...) {}
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
            entry["map_max_x"]  = b.map_max_x;
            entry["map_max_y"]  = b.map_max_y;
            j[std::to_string(mapId)] = entry;
        }
    }
    try {
        std::filesystem::create_directories(m_dataDir);
        std::ofstream f(path);
        f << j.dump(2);
    } catch (...) {}
}
