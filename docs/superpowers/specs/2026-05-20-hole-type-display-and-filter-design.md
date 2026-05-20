# Hole Type Display and Map Filter

**Date:** 2026-05-20

## Goal

Show the fishing hole type (Shore, Offshore, River, etc.) on the fish details pane, filter the map to only show holes of that type when a fish is selected, and remove the now-redundant Freshwater/Saltwater/Special filter dropdown.

## Background

`HoleWater` (from `HoleLocations.h`) is Tekkit's per-hole water type enum. `HOLE_LOCATION_TABLE` has 3,660 holes across 54 maps, each tagged with a `HoleWater` value. Many maps contain multiple hole types (e.g. Seitung Province has Shore, Offshore, Shinota, Wreckage). The existing `Fish.water` (`WaterType`: Freshwater/Saltwater/Special) is a coarser field kept for other purposes and is not replaced by this work.

## Scope

- `HoleLocations.h` — add `Any = 255` sentinel to `HoleWater` enum
- `FishData.h` — add `holeType` field to `Fish` struct
- `FishData.cpp` — populate `holeType` on all 287 `FISH_TABLE` entries
- `dllmain.cpp` — add "Hole" row to details pane; remove water filter dropdown
- `MapPanel.cpp` — filter holes in `RenderHoles` by `holeType`

## Data Model

### `HoleWater::Any` sentinel

Add `Any = 255` to the `HoleWater` enum in `HoleLocations.h`. Used for fish that appear in any hole type on their map (e.g. World fish, Open Water collections) — no map filtering applied.

### `Fish.holeType`

Add `HoleWater holeType` to the `Fish` struct in `FishData.h`. Populated per fish in `FishData.cpp` based on GW2 wiki data.

**Derivation rules:**

- Core content fish with unambiguous collection names map directly:
  - Lake Fish (Ascalon) → `Lake`
  - River Fish (Kryta) → `River`
  - Freshwater Fish (Maguuma Jungle) → `Freshwater`
  - Boreal Fish (Shiverpeak Mountains) → `Boreal`
  - Offshore Fish (Ruins of Orr) → `Offshore`
  - Desert Fish (Crystal Desert / Desert Isles) → `Desert`
  - Volcanic Fish (Ring of Fire) → `Volcanic`
  - Shore Fish (Seitung Province / Desert Isles) → `Shore`

- EoD and newer zones have multiple hole types per map and non-obvious mappings.
  Verify each fish against the wiki. Where Tekkit's label differs from GW2's
  in-game name (e.g. NKC "Offshore" collection uses Channel/Coastal holes in
  the marker pack), use the `HoleWater` value that matches the actual holes
  present on that map in `HOLE_LOCATION_TABLE`.

- World / Open Water / Saltwater collection fish → `Any`.

- Any fish where the correct type cannot be determined → `Any` (safe default:
  shows all holes on the map).

## Details Pane

In `RenderFishDetails` (`dllmain.cpp`), add a "Hole" row immediately after the existing "Water" row:

```
row("Hole", f.holeType == HoleWater::Any ? "-" : HoleWaterName(f.holeType));
```

`HoleWaterName` is already declared in `HoleLocations.h`.

## Map Filtering

`RenderHoles` in `MapPanel.cpp` receives `selectedFishIdx` but currently ignores it. Change it to:

1. Look up `FISH_TABLE[selectedFishIdx].holeType` (guard against out-of-range index).
2. If `holeType != HoleWater::Any`, add `&& h.water == holeType` to the per-hole skip condition.
3. If `holeType == Any`, show all holes on the map (existing behaviour).

`FishData.h` is already included transitively via `MapPanel.h` → `FishData.h`, so `FISH_TABLE` and `HoleWater` are accessible in `MapPanel.cpp`.

## Remove Water Type Filter

In `dllmain.cpp`:

- Remove the Freshwater / Saltwater / Special combo box from the filter bar UI.
- Remove its backing variable (find by searching `g_Filter` variables near the filter bar).
- Remove the corresponding predicate check in the fish visibility function.

`WaterType`, `Fish.water`, and the "Water" row in the details pane are **not** changed.

## Non-Goals

- No changes to `WaterType` or the "Water" details-pane row.
- No bitmask support for fish that span multiple hole types — `Any` is the safe fallback.
- No filtering of waypoints or sector labels by hole type.
