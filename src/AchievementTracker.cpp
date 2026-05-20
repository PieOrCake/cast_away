#include "AchievementTracker.h"
#include "../include/HoardAndSeekAPI.h"
#include <cstring>

AchievementTracker g_AchTracker;

#define EV_CAST_AWAY_ACH_RESPONSE "EV_CAST_AWAY_ACH_RESPONSE"
#define EV_ALERT_ACHIEVEMENT_COMPLETED "EV_ALERT:AchievementCompleted"

struct AlertAchPayload {
    unsigned int ID;
};

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void AchievementTracker::Init(AddonAPI_t* api) {
    m_api = api;

    // Seed collections from COLLECTION_TABLE
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (int i = 0; i < COLLECTION_COUNT; i++) {
            const FishingCollection& c = COLLECTION_TABLE[i];
            if (c.achievementId == 0) continue;
            CollectionState& st = m_collections[c.achievementId];
            st.totalFish   = c.totalFish;
            st.caughtCount = 0;
            st.done        = false;
            st.bitsKnown   = false;
        }
    }

    api->Events_Subscribe(EV_HOARD_PONG,                   OnHoardPong);
    api->Events_Subscribe(EV_HOARD_DATA_UPDATED,           OnHoardDataUpdated);
    api->Events_Subscribe(EV_CAST_AWAY_ACH_RESPONSE,       OnHoardAchResponse);
    api->Events_Subscribe(EV_ALERT_ACHIEVEMENT_COMPLETED,  OnAlertAchievementCompleted);

    // Ping H&S — if it's loaded it will reply with EV_HOARD_PONG
    api->Events_Raise(EV_HOARD_PING, nullptr);
}

void AchievementTracker::Shutdown() {
    if (!m_api) return;
    m_api->Events_Unsubscribe(EV_HOARD_PONG,                  OnHoardPong);
    m_api->Events_Unsubscribe(EV_HOARD_DATA_UPDATED,          OnHoardDataUpdated);
    m_api->Events_Unsubscribe(EV_CAST_AWAY_ACH_RESPONSE,      OnHoardAchResponse);
    m_api->Events_Unsubscribe(EV_ALERT_ACHIEVEMENT_COMPLETED, OnAlertAchievementCompleted);
    m_api = nullptr;
}

// ---------------------------------------------------------------------------
// FlushPendingQuery — call from render thread only
// ---------------------------------------------------------------------------

void AchievementTracker::FlushPendingQuery() {
    // Periodic re-poll: re-query every 10 minutes while H&S is active
    if (hoarded && !queryPending) {
        time_t now = time(nullptr);
        if (m_lastQueryTime == 0 || now - m_lastQueryTime >= 600)
            queryPending = true;
    }

    if (!queryPending) return;
    queryPending = false;
    m_lastQueryTime = time(nullptr);
    QueryAllCollections();
}

// ---------------------------------------------------------------------------
// IsCaught
// ---------------------------------------------------------------------------

bool AchievementTracker::IsCaught(int fishIdx) const {
    if (fishIdx < 0 || fishIdx >= FISH_COUNT) return false;
    const Fish& f = FISH_TABLE[fishIdx];
    if (f.achievementId == 0) return false;

    std::lock_guard<std::mutex> lk(m_mu);
    auto it = m_collections.find(f.achievementId);
    if (it == m_collections.end()) return false;
    const CollectionState& st = it->second;
    if (!st.bitsKnown) return false;
    if (f.bitIndex >= 64) return false;
    return st.caughtBits[f.bitIndex];
}

const CollectionState* AchievementTracker::GetCollection(uint32_t achievementId) const {
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = m_collections.find(achievementId);
    if (it == m_collections.end()) return nullptr;
    return &it->second;
}

// ---------------------------------------------------------------------------
// QueryAllCollections — must NOT be holding m_mu when called
// ---------------------------------------------------------------------------

void AchievementTracker::QueryAllCollections() {
    if (!m_api) return;

    HoardQueryAchievementRequest req{};
    req.api_version = HOARD_API_VERSION;
    strncpy(req.requester, "Cast Away", sizeof(req.requester) - 1);
    strncpy(req.response_event, EV_CAST_AWAY_ACH_RESPONSE, sizeof(req.response_event) - 1);
    req.id_count = 0;

    // Collect all non-zero achievement IDs
    for (int i = 0; i < COLLECTION_COUNT; i++) {
        if (COLLECTION_TABLE[i].achievementId != 0 && req.id_count < 200) {
            req.ids[req.id_count++] = COLLECTION_TABLE[i].achievementId;
        }
    }

    if (req.id_count == 0) return;

    // CRITICAL: Do NOT hold m_mu here — response arrives synchronously inside this call
    m_api->Events_Raise(EV_HOARD_QUERY_ACHIEVEMENT, &req);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void AchievementTracker::OnHoardPong(void* args) {
    if (!args) return;
    auto* pong = static_cast<HoardPongPayload*>(args);
    g_AchTracker.hoarded = true;
    if (pong->has_data) {
        g_AchTracker.queryPending = true;
    }
}

void AchievementTracker::OnHoardDataUpdated(void* args) {
    (void)args;
    g_AchTracker.hoarded      = true;
    g_AchTracker.queryPending = true;
}

void AchievementTracker::OnHoardAchResponse(void* args) {
    if (!args) return;
    auto* resp = static_cast<HoardQueryAchievementResponse*>(args);

    if (resp->status != HOARD_STATUS_OK) {
        delete resp;
        return;
    }

    {
        std::lock_guard<std::mutex> lk(g_AchTracker.m_mu);
        for (uint32_t i = 0; i < resp->entry_count; i++) {
            const HoardAchievementEntry& e = resp->entries[i];
            auto it = g_AchTracker.m_collections.find(e.id);
            if (it == g_AchTracker.m_collections.end()) continue;

            CollectionState& st = it->second;
            // Clear bits then repopulate
            memset(st.caughtBits, 0, sizeof(st.caughtBits));
            for (uint32_t b = 0; b < e.bit_count && b < 64; b++) {
                if (e.bits[b] < 64) {
                    st.caughtBits[e.bits[b]] = true;
                }
            }
            st.bitsKnown = true;

            // Recount caughtCount
            uint8_t count = 0;
            for (int b = 0; b < 64; b++) {
                if (st.caughtBits[b]) count++;
            }
            st.caughtCount = count;
            st.done = e.done || (st.totalFish > 0 && count >= st.totalFish);
        }
    }

    delete resp;
}

void AchievementTracker::OnAlertAchievementCompleted(void* args) {
    if (!args) return;
    auto* payload = static_cast<AlertAchPayload*>(args);

    std::lock_guard<std::mutex> lk(g_AchTracker.m_mu);
    auto it = g_AchTracker.m_collections.find(payload->ID);
    if (it == g_AchTracker.m_collections.end()) return;

    CollectionState& st = it->second;
    if (st.done) return;

    st.caughtCount++;
    if (st.totalFish > 0 && st.caughtCount >= st.totalFish) {
        st.done = true;
        // Mark all bits caught optimistically
        memset(st.caughtBits, 1, sizeof(st.caughtBits));
        st.bitsKnown = true;
    }
    // Do NOT re-query — the API won't reflect the change immediately
}
