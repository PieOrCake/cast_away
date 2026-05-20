#include "FishData.h"

static const char* AMBERGRIS_ICON =
    "https://render.guildwars2.com/file/C113C406281D94BB560701DF350BCA9AF592AFE9/2597025.png";

// Guaranteed Ambergris singleton (for Flawless Fillet fish not in BONUS_TABLE)
static const BonusItem s_ambergrisGuaranteed = {
    96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, false
};

struct BonusEntry { uint32_t fishItemId; BonusItem bonus; };
static const BonusEntry BONUS_TABLE[] = {
    // fishItemId           bonusItemId  name                             icon           isChance
    { 96318U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // Mega Prawn
    { 95961U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // Krytan Crawfish
    { 96513U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // King Crab
};

// Any Flawless Fillet (95673) fish without an explicit Ambergris entry gets one implicitly.
static bool HasImplicitAmbergris(uint32_t fishItemId) {
    for (const auto& e : BONUS_TABLE)
        if (e.fishItemId == fishItemId && e.bonus.itemId == 96347U)
            return false; // already has an explicit Ambergris entry
    for (int i = 0; i < FISH_COUNT; ++i)
        if (FISH_TABLE[i].itemId == fishItemId && FISH_TABLE[i].filletItemId == 95673U)
            return true;
    return false;
}

int GetBonusItemCount(uint32_t fishItemId) {
    int n = 0;
    for (const auto& e : BONUS_TABLE)
        if (e.fishItemId == fishItemId) n++;
    if (HasImplicitAmbergris(fishItemId)) n++;
    return n;
}

const BonusItem* GetBonusItem(uint32_t fishItemId, int index) {
    int n = 0;
    for (const auto& e : BONUS_TABLE) {
        if (e.fishItemId == fishItemId) {
            if (n == index) return &e.bonus;
            n++;
        }
    }
    if (HasImplicitAmbergris(fishItemId)) {
        if (n == index) return &s_ambergrisGuaranteed;
    }
    return nullptr;
}
