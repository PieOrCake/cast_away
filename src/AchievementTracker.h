#pragma once
#include <cstdint>
#include <ctime>
#include <unordered_map>
#include <mutex>
#include "nexus/Nexus.h"
#include "FishData.h"

struct CollectionState {
    uint8_t  caughtCount = 0;
    uint8_t  totalFish   = 0;
    bool     done        = false;
    bool     bitsKnown   = false;
    bool     caughtBits[64] = {};
};

class AchievementTracker {
public:
    void Init(AddonAPI_t* api);
    void Shutdown();

    // Call from render thread only — safe to call Events_Raise here
    void FlushPendingQuery();

    bool IsCaught(int fishIdx) const;
    const CollectionState* GetCollection(uint32_t achievementId) const;

    bool hoarded      = false;
    bool queryPending = false;

private:
    void QueryAllCollections();
    static void OnHoardPong(void* args);
    static void OnHoardDataUpdated(void* args);
    static void OnHoardAchResponse(void* args);
    static void OnAlertAchievementCompleted(void* args);

    AddonAPI_t* m_api = nullptr;
    mutable std::mutex m_mu;
    std::unordered_map<uint32_t, CollectionState> m_collections;
    time_t m_lastQueryTime = 0;
};

extern AchievementTracker g_AchTracker;
