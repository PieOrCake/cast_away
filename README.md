# Cast Away

A fishing companion addon for **Guild Wars 2**, built on the [Nexus](https://raidcore.gg/Nexus) addon framework.

Track every catchable fish, find fishing holes on an interactive map, and never miss the right time-of-day window again.

## AI Notice

This addon has been largely created using Claude. I understand that some folks have a moral, financial or political objection to creating software using an LLM. I just wanted to make a useful tool for the GW2 community, and this was the only way I could do it.

If an LLM creating software upsets you, then perhaps this repo isn't for you. Move on, and enjoy your day.

## Features

- **307 fish** — Core through Visions of Eternity, each with bait, time-of-day, fishing hole type, fillet, and TP price.
- **Interactive map** — 3,660+ fishing holes across 54 maps, GW2 waypoints, pan and zoom. When a fish is selected the map navigates to its region and filters holes to the matching type (Shore, Offshore, Lake, etc.).
- **Grouped database view** — browse all fish flat or switch to collection groupings, each with a caught/total progress bar. Filters (bait, time, "Hide caught") apply in both views.
- **Achievement tracking** — per-collection progress with green progress bars, driven by optional Hoard & Seek / Events:Alerts integration.
- **Day/night tracker** — scrolling Tyrian-time bar with phase countdown. Click the HUD overlay to open the main window.
- **Favourite fish alerts** — notifies you before a favourite's time window opens, with a direct Open Map button.
- **Fish details** — bait, time, hole type, water type, collection, catch status, fillet price, and all bonus drops (guaranteed and chance).
- **Animated fishtank background** — fish, shark, and seaweed across all panels (optional, toggle in settings).

## Installation

1. Install [Nexus](https://raidcore.gg/Nexus) into your Guild Wars 2 directory.
2. Drop `CastAway.dll` into `<GW2>\addons\`.
3. Launch the game — Cast Away appears as a window button in the Nexus quick-access.

## Building from source

Cross-compiles from Linux to Windows via MinGW. Requires CMake ≥ 3.20.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Output: `build/CastAway.dll`.

## Optional companions

- **[Hoard & Seek](https://raidcore.gg)** — drives the catch-progress tracking.
- **[Events:Alerts](https://raidcore.gg)** — enables real-time optimistic updates when you reel a fish in.

Neither is required — Cast Away gracefully falls back to API-only data if they're missing.

---
## License

MIT

## Third-party credits & licenses

- **Fishing-hole coordinates** — sourced from the *All-In-One* marker pack by **[Tekkit's Workshop](https://www.tekkitsworkshop.net/markers/all-in-one-marker-pack)**. The pack itself is free to download and the underlying coordinates are factual game-world data; credit and link are given here in gratitude for the curation effort.
- **Waypoint and POI metadata** — fetched live from the official **[Guild Wars 2 API](https://wiki.guildwars2.com/wiki/API:Main)** (`/v2/continents/1/floors/*/regions`).
- **Map tiles** — served by ArenaNet's tile service (`tiles.guildwars2.com`).
- **Icons** — fish, bait, fillet, bonus-drop, and waypoint icons rendered from `render.guildwars2.com`. All Guild Wars 2 assets &copy; ArenaNet / NCSOFT.
- **[Dear ImGui](https://github.com/ocornut/imgui)** — UI rendering. MIT License.
- **[nlohmann/json](https://github.com/nlohmann/json)** — JSON parsing. MIT License.
- **[Nexus](https://raidcore.gg/Nexus)** by Raidcore — addon framework providing the load/unload hooks, texture and MumbleLink APIs.

Guild Wars 2 &copy; ArenaNet, LLC. Cast Away is an unofficial fan-made addon and is not affiliated with or endorsed by ArenaNet or NCSOFT.
