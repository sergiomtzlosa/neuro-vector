// Neuro-Vector — HOW TO PLAY screen: mission, controls, bestiary, scoring.
#pragma once
#include "../core/Common.h"

namespace nv {

class LineRenderer;
struct Palette;

void drawHelpScreen(LineRenderer& lr, float w, float h, const Palette& pal);

} // namespace nv
