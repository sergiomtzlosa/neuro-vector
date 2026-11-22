// Neuro-Vector — mock operator/diagnostic menu (NEURO-VECTOR DIAGNOSTICS v1.4).
#pragma once
#include "../core/Common.h"

namespace nv {

class LineRenderer;
struct Settings;

void drawOperatorMenu(LineRenderer& lr, float w, float h, float t, const Settings& s,
                      int scoreCount, int sel);

} // namespace nv
