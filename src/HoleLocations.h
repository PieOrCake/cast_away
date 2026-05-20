#pragma once
#include <cstdint>

// Generated from Tekkit's Workshop ALL-IN-ONE marker pack (tw_core_fishing.xml).
// Each entry is one in-game fishing-hole position.
// game_x / game_z match MumbleLink fAvatarPosition[0]/[2] (metres).

enum class HoleWater : uint8_t {
    Boreal = 0,
    Cavern = 1,
    Channel = 2,
    Coastal = 3,
    Deep = 4,
    Desert = 5,
    Freshwater = 6,
    Grotto = 7,
    Lake = 8,
    Lakeboreal = 9,
    Lakecoastal = 10,
    Lakenoxious = 11,
    Lakeriver = 12,
    Noxious = 13,
    Offshore = 14,
    Polluted = 15,
    Quarry = 16,
    River = 17,
    Saltwater = 18,
    Shinota = 19,
    Shore = 20,
    Volcanic = 21,
    Volcanicnoxious = 22,
    Wreckage = 23,
};

struct HoleLocation {
    uint32_t  mapId;
    float     game_x;
    float     game_z;
    HoleWater water;
};

extern const HoleLocation HOLE_LOCATION_TABLE[];
extern const int          HOLE_LOCATION_COUNT;

const char* HoleWaterName(HoleWater w);
