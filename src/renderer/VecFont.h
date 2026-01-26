// Neuro-Vector — stroke vector font (authentic XY-monitor lettering).
//
// Glyphs live on a 4x6 grid: x in [0,4], y in [0,6] with baseline y=0.
// Encoded per glyph: number of strokes, then per stroke: point count + flat x,y pairs.
#pragma once
#include "../core/Common.h"
#include <vector>

namespace nv {

class LineRenderer;

class VecFont {
public:
    static void init();
    static float measure(const char* text, float size);
    static float width(const char* text, float size); // alias
    enum Align { LEFT, CENTER, RIGHT };
    static void draw(LineRenderer& lr, const char* text, float x, float y, float size,
                     Color c, int align = LEFT, int glow = 0);
};

} // namespace nv
