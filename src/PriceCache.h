#pragma once
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>

struct PriceInfo {
    uint32_t item_id    = 0;
    int      buy_price  = 0;  // highest buy order, copper
    int      sell_price = 0;  // lowest sell listing, copper
    bool     tradeable  = false;
    bool     fetched    = false;
    std::chrono::steady_clock::time_point fetched_at;
};

// Format copper value as "1g 23s 45c" string
std::string FormatCoins(int copper);

class PriceCache {
public:
    // Request prices for up to 2 item IDs (fish + fillet).
    // No-op if already fresh (within TTL). Spawns background thread.
    void Request(uint32_t itemId1, uint32_t itemId2 = 0);

    // Returns cached price or nullptr. Never blocks.
    const PriceInfo* Get(uint32_t itemId) const;

private:
    static void FetchWorker(std::vector<uint32_t> ids, PriceCache* cache);
    static const int TTL_SECONDS = 300; // 5 minutes

    mutable std::mutex m_mu;
    std::unordered_map<uint32_t, PriceInfo> m_prices;
    std::unordered_map<uint32_t, bool>      m_inflight; // ids currently being fetched
};

extern PriceCache g_Prices;
