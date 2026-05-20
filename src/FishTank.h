#pragma once
#include "../lib/imgui/imgui.h"

// Ocean gradient, sand, caustics, seaweed, particles — clipped to mn/mx.
// Call inside the card child window after BeginChild.
void DrawFishtankBg(ImDrawList* dl, ImVec2 mn, ImVec2 mx);

// Swimming fish and crab — call on the PARENT window draw list after both
// child windows have ended, passing bounds that span the full fish area
// (card grid + details pane) so creatures swim across both panels.
void DrawFishtankCreatures(ImDrawList* dl, ImVec2 mn, ImVec2 mx);
