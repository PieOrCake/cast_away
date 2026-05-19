#pragma once
#include <cstdint>
#include <ctime>

// Bait types — index 0 = Any (no special bait)
enum class BaitType : uint8_t {
    Any         = 0,
    BorrowedBait= 1,
    FishEgg     = 2,
    FreshwaterMinnow = 3,
    GlowWorm    = 4,
    HaijuMinnow = 5,
    LavaBeetle  = 6,
    Leech       = 7,
    LightningBug= 8,
    Mackerel    = 9,
    Nightcrawler= 10,
    RamsHornSnail= 11,
    Sardine     = 12,
    Scorpion    = 13,
    Shrimpling  = 14,
    SparkflyLarva= 15,
};

enum class TimeOfDay : uint8_t {
    Any   = 0,
    Dawn  = 1,  // covers Dusk/Dawn
    Day   = 2,
    Dusk  = 3,
    Night = 4,
};

enum class WaterType : uint8_t {
    Freshwater = 0,
    Saltwater  = 1,
    Special    = 2,
};

struct Fish {
    const char* name;
    const char* map;           // GW2 region name
    const char* region;        // Expansion label
    BaitType    bait;
    TimeOfDay   time;
    WaterType   water;
    const char* collection;    // Achievement collection name
    uint32_t    achievementId; // 0 = TBD (Task 10)
    uint8_t     bitIndex;      // 0 = TBD (Task 10)
    uint32_t    itemId;        // GW2 item ID (0 if unknown)
    const char* iconUrl;       // render.guildwars2.com URL
    uint32_t    filletItemId;  // 0 if none
    const char* filletName;    // nullptr if none
    const char* filletIconUrl; // nullptr if none
    const char* masteryRequired; // nullptr if none
    const char* wikiSlug;
};

struct Waypoint {
    const char* name;
    const char* chatLink;
    float       cont_x;
    float       cont_y;
};

struct FishingHole {
    const char* name;
    const char* map;
    uint32_t    mapId;
    float       game_x;
    float       game_z;
    uint16_t    waypointIdx;
    uint16_t    fishIds[16];
    uint8_t     fishCount;
};

struct FishingCollection {
    const char* name;
    uint32_t    achievementId;
    uint8_t     totalFish;
    const char* iconUrl;
};

extern const char*             BAIT_NAMES[];
extern const int               BAIT_COUNT;
extern const Fish              FISH_TABLE[];
extern const int               FISH_COUNT;
extern const Waypoint          WAYPOINT_TABLE[];
extern const int               WAYPOINT_COUNT;
extern const FishingHole       HOLE_TABLE[];
extern const int               HOLE_COUNT;
extern const FishingCollection COLLECTION_TABLE[];
extern const int               COLLECTION_COUNT;

inline const char* TimeOfDayName(TimeOfDay t) {
    switch (t) {
        case TimeOfDay::Dawn:  return "Dawn/Dusk";
        case TimeOfDay::Day:   return "Daytime";
        case TimeOfDay::Dusk:  return "Dusk/Dawn";
        case TimeOfDay::Night: return "Nighttime";
        default:               return "Any";
    }
}

inline const char* WaterTypeName(WaterType w) {
    switch (w) {
        case WaterType::Saltwater:  return "Saltwater";
        case WaterType::Special:    return "Special";
        default:                    return "Freshwater";
    }
}

// GW2 day-night cycle: 4320 seconds total
// Dawn=0-300, Day=300-1800, Dusk=1800-2100, Night=2100-4320
inline uint32_t GetTyrianSeconds() {
    return (uint32_t)(time(nullptr) % 4320);
}

inline float GetTyrianHour() {
    return (float)(time(nullptr) % 4320) / 180.0f;
}

inline TimeOfDay GetCurrentTimeOfDay() {
    uint32_t s = GetTyrianSeconds();
    if (s < 300)  return TimeOfDay::Dawn;
    if (s < 1800) return TimeOfDay::Day;
    if (s < 2100) return TimeOfDay::Dusk;
    return TimeOfDay::Night;
}

inline uint32_t SecondsUntilNextSlot() {
    uint32_t s = GetTyrianSeconds();
    if (s < 300)  return 300 - s;
    if (s < 1800) return 1800 - s;
    if (s < 2100) return 2100 - s;
    return 4320 - s;
}

inline uint32_t SecondsUntilPhase(TimeOfDay phase) {
    uint32_t s = GetTyrianSeconds();
    uint32_t start;
    switch (phase) {
        case TimeOfDay::Dawn:  start = 0;    break;
        case TimeOfDay::Day:   start = 300;  break;
        case TimeOfDay::Dusk:  start = 1800; break;
        case TimeOfDay::Night: start = 2100; break;
        default: return 0;
    }
    return (start > s) ? (start - s) : (4320 - s + start);
}

#define CAST_AWAY_ADDON_NAME "Cast Away"
