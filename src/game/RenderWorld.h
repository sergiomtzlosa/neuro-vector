// Neuro-Vector — draws the world as glowing wireframes (Quadrascan style).
#pragma once
#include "../core/Common.h"
#include "../renderer/LineRenderer.h"

namespace nv {

class World;
struct LevelParams;

struct Palette {
    Color tunnel, lane, rim, player, zap, ui, dim;
    Color enemy[8];
};

Palette makePalette(const LevelParams& P);

// alpha: interpolation factor (0..1) for camera/entity motion smoothing.
void drawWorld(LineRenderer& lr, const World& w, const Palette& pal,
               float width, float height, float alpha, float shakeX, float shakeY);

void drawHud(LineRenderer& lr, const World& w, float width, float height, float blink, bool demo);

// draws a single enemy archetype shape (used by the help screen bestiary)
void drawEnemyGlyph(LineRenderer& lr, int kind, Vec2 p, float s, Color c, float t);

// vector spaceship icon pointing +x (used for the HUD lives row)
void drawShipIcon(LineRenderer& lr, Vec2 p, float s, Color c);

} // namespace nv
