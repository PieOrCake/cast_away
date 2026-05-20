#include "FishData.h"

static const char* AMBERGRIS_ICON =
    "https://render.guildwars2.com/file/C113C406281D94BB560701DF350BCA9AF592AFE9/2597025.png";

struct BonusEntry { uint32_t fishItemId; BonusItem bonus; };
static const BonusEntry BONUS_TABLE[] = {
    // fishItemId           bonusItemId  name                         icon           isChance
    { 96318U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // Mega Prawn
    { 95961U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // Krytan Crawfish
    { 96513U, { 96347U, "Chunk of Ancient Ambergris", AMBERGRIS_ICON, true  } }, // King Crab
};

const BonusItem* GetBonusItem(uint32_t fishItemId) {
    for (const auto& e : BONUS_TABLE)
        if (e.fishItemId == fishItemId) return &e.bonus;
    return nullptr;
}
