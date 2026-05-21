#pragma once
#include <cstdint>
#include "HoleLocations.h"
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
    HoleWater   holeType = HoleWater::Any;
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

const char* GetFishRarity(uint32_t itemId);

// Returns the recommended fishing power for a fish given its region (Fish.map)
// and holeType. 0 = unknown / not displayed.
int GetRecommendedPower(const char* fishMap, HoleWater holeType);

struct BaitInfo {
    uint32_t    itemId;
    const char* iconUrl;
};
// Returns icon/item info for a bait type, or nullptr for Any/BorrowedBait.
const BaitInfo* GetBaitInfo(BaitType b);

struct BonusItem {
    uint32_t    itemId;
    const char* name;
    const char* iconUrl;
    bool        isChance; // true = chance drop, false = guaranteed
};

// Returns count of bonus drops for a fish (keyed by fish itemId).
int GetBonusItemCount(uint32_t fishItemId);
// Returns bonus drop at given index, or nullptr if out of range.
const BonusItem* GetBonusItem(uint32_t fishItemId, int index = 0);

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

// GW2 day-night cycle: 7200 sec (120 min). UTC 00:00 = Tyrian 00:00 (mid-Night).
// 1 Tyrian hour = 300 real seconds = 5 real minutes.
// Phase boundaries on the Tyrian clock:
//   Night 21:00–05:00
//   Dawn  05:00–06:00
//   Day   06:00–20:00
//   Dusk  20:00–21:00
// As cycle-seconds (0 = Tyrian midnight):
//   0    – 1500 Night (early)
//   1500 – 1800 Dawn
//   1800 – 6000 Day
//   6000 – 6300 Dusk
//   6300 – 7200 Night (late)

static const uint32_t TYRIAN_CYCLE   = 7200;
static const uint32_t TY_DAWN_START  = 1500;
static const uint32_t TY_DAY_START   = 1800;
static const uint32_t TY_DUSK_START  = 6000;
static const uint32_t TY_NIGHT_START = 6300;

inline uint32_t GetTyrianSeconds() {
    return (uint32_t)(time(nullptr) % TYRIAN_CYCLE);
}

// Tyrian clock hour (0..24). UTC 00:00 = Tyrian 00:00.
inline float GetTyrianHour() {
    return (float)GetTyrianSeconds() / 300.0f;
}

inline TimeOfDay GetCurrentTimeOfDay() {
    uint32_t s = GetTyrianSeconds();
    if (s < TY_DAWN_START)  return TimeOfDay::Night; // 00:00–05:00 Tyrian
    if (s < TY_DAY_START)   return TimeOfDay::Dawn;
    if (s < TY_DUSK_START)  return TimeOfDay::Day;
    if (s < TY_NIGHT_START) return TimeOfDay::Dusk;
    return TimeOfDay::Night;                         // 21:00–24:00 Tyrian
}

inline TimeOfDay GetNextPhase() {
    switch (GetCurrentTimeOfDay()) {
        case TimeOfDay::Night: return TimeOfDay::Dawn;
        case TimeOfDay::Dawn:  return TimeOfDay::Day;
        case TimeOfDay::Day:   return TimeOfDay::Dusk;
        case TimeOfDay::Dusk:  return TimeOfDay::Night;
        default:               return TimeOfDay::Day;
    }
}

inline uint32_t SecondsUntilNextSlot() {
    uint32_t s = GetTyrianSeconds();
    if (s < TY_DAWN_START)  return TY_DAWN_START  - s;
    if (s < TY_DAY_START)   return TY_DAY_START   - s;
    if (s < TY_DUSK_START)  return TY_DUSK_START  - s;
    if (s < TY_NIGHT_START) return TY_NIGHT_START - s;
    // Late night (6300–7200): next boundary is Dawn at 1500 of next cycle.
    return (TYRIAN_CYCLE - s) + TY_DAWN_START;
}

inline uint32_t SecondsUntilPhase(TimeOfDay phase) {
    if (phase == TimeOfDay::Any) return 0;
    uint32_t s = GetTyrianSeconds();
    uint32_t start = 0;
    switch (phase) {
        case TimeOfDay::Dawn:  start = TY_DAWN_START;  break;
        case TimeOfDay::Day:   start = TY_DAY_START;   break;
        case TimeOfDay::Dusk:  start = TY_DUSK_START;  break;
        case TimeOfDay::Night: start = TY_NIGHT_START; break;
        default:               return 0;
    }
    return (start > s) ? (start - s) : (TYRIAN_CYCLE - s + start);
}

#define CAST_AWAY_ADDON_NAME "Cast Away"
