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
        case TimeOfDay::Dawn:  return "Dawn";
        case TimeOfDay::Day:   return "Day";
        case TimeOfDay::Dusk:  return "Dusk";
        case TimeOfDay::Night: return "Night";
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

// GW2 day-night cycle: 4320 real seconds (72 minutes) total.
// Boundaries per wiki: Dawn 5 min, Day 40 min, Dusk 5 min, Night 22 min.
// Dawn=0-300, Day=300-2700, Dusk=2700-3000, Night=3000-4320
//
// EPOCH NOTE: The displayed Tyrian hour may not match GW2's /time command.
// If it drifts, adjust TYRIAN_EPOCH_OFFSET (seconds to subtract from UTC).
// Empirically ~858 has been observed; verify in-game and tune as needed.
static const uint32_t TYRIAN_EPOCH_OFFSET = 858;

inline uint32_t GetTyrianSeconds() {
    time_t t = time(nullptr);
    return (uint32_t)((t >= (time_t)TYRIAN_EPOCH_OFFSET
        ? t - (time_t)TYRIAN_EPOCH_OFFSET : t) % 4320);
}

inline float GetTyrianHour() {
    return (float)GetTyrianSeconds() / 180.0f;
}

inline TimeOfDay GetCurrentTimeOfDay() {
    uint32_t s = GetTyrianSeconds();
    if (s < 300)  return TimeOfDay::Dawn;
    if (s < 2700) return TimeOfDay::Day;
    if (s < 3000) return TimeOfDay::Dusk;
    return TimeOfDay::Night;
}

inline TimeOfDay GetNextPhase() {
    switch (GetCurrentTimeOfDay()) {
        case TimeOfDay::Dawn:  return TimeOfDay::Day;
        case TimeOfDay::Day:   return TimeOfDay::Dusk;
        case TimeOfDay::Dusk:  return TimeOfDay::Night;
        default:               return TimeOfDay::Dawn;
    }
}

inline uint32_t SecondsUntilNextSlot() {
    uint32_t s = GetTyrianSeconds();
    if (s < 300)  return 300  - s;
    if (s < 2700) return 2700 - s;
    if (s < 3000) return 3000 - s;
    return 4320 - s;
}

inline uint32_t SecondsUntilPhase(TimeOfDay phase) {
    uint32_t s = GetTyrianSeconds();
    const uint32_t starts[] = { 0, 0, 300, 2700, 3000 }; // Any, Dawn, Day, Dusk, Night
    if (phase == TimeOfDay::Any) return 0;
    uint32_t start = starts[(int)phase];
    return (start > s) ? (start - s) : (4320 - s + start);
}

#define CAST_AWAY_ADDON_NAME "Cast Away"
