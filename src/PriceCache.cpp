#include "PriceCache.h"
#include <windows.h>
#include <wininet.h>
#include <nlohmann/json.hpp>
#include <thread>

PriceCache g_Prices;

std::string FormatCoins(int copper) {
    if (copper <= 0) return "0c";
    std::string r;
    int gold   = copper / 10000;
    int silver = (copper % 10000) / 100;
    int cop    = copper % 100;
    if (gold)           r += std::to_string(gold)   + "g ";
    if (silver || gold) r += std::to_string(silver) + "s ";
    r += std::to_string(cop) + "c";
    return r;
}

static std::string HttpGet(const std::string& url) {
    HINTERNET hNet = InternetOpenA("CastAway/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return {};
    HINTERNET hUrl = InternetOpenUrlA(hNet, url.c_str(), NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return {}; }
    std::string r; char buf[4096]; DWORD rd;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &rd) && rd > 0) r.append(buf, rd);
    InternetCloseHandle(hUrl); InternetCloseHandle(hNet);
    return r;
}

void PriceCache::Request(uint32_t id1, uint32_t id2) {
    std::vector<uint32_t> toFetch;
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (uint32_t id : {id1, id2}) {
            if (id == 0) continue;
            if (m_inflight.count(id)) continue;
            auto it = m_prices.find(id);
            if (it != m_prices.end() && it->second.fetched) {
                auto age = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second.fetched_at).count();
                if (age < TTL_SECONDS) continue;
            }
            m_inflight[id] = true;
            toFetch.push_back(id);
        }
    }
    if (!toFetch.empty())
        std::thread(FetchWorker, toFetch, this).detach();
}

const PriceInfo* PriceCache::Get(uint32_t id) const {
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = m_prices.find(id);
    return (it != m_prices.end() && it->second.fetched) ? &it->second : nullptr;
}

void PriceCache::FetchWorker(std::vector<uint32_t> ids, PriceCache* cache) {
    std::string param;
    for (auto id : ids) {
        if (!param.empty()) param += ",";
        param += std::to_string(id);
    }
    std::string json = HttpGet(
        "https://api.guildwars2.com/v2/commerce/prices?ids=" + param);
    std::lock_guard<std::mutex> lk(cache->m_mu);
    if (json.empty()) {
        for (auto id : ids) cache->m_inflight.erase(id);
        return;
    }
    try {
        auto j = nlohmann::json::parse(json);
        for (const auto& e : j) {
            uint32_t id = e.value("id", 0u);
            if (id == 0) continue;
            PriceInfo info;
            info.item_id    = id;
            info.tradeable  = true;
            info.fetched    = true;
            info.fetched_at = std::chrono::steady_clock::now();
            if (e.contains("buys"))  info.buy_price  = e["buys"].value("unit_price", 0);
            if (e.contains("sells")) info.sell_price = e["sells"].value("unit_price", 0);
            cache->m_prices[id] = info;
            cache->m_inflight.erase(id);
        }
        // Mark IDs not returned by API as non-tradeable so we don't retry immediately
        for (auto id : ids) {
            if (!cache->m_prices.count(id) || !cache->m_prices[id].fetched) {
                PriceInfo info;
                info.item_id    = id;
                info.fetched    = true;
                info.tradeable  = false;
                info.fetched_at = std::chrono::steady_clock::now();
                cache->m_prices[id] = info;
            }
            cache->m_inflight.erase(id);
        }
    } catch (...) {
        for (auto id : ids) cache->m_inflight.erase(id);
    }
}
