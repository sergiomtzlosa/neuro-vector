#include "HelpScreen.h"
#include "../renderer/LineRenderer.h"
#include "../renderer/VecFont.h"
#include "../game/RenderWorld.h"
#include <cstdio>

namespace nv {

void drawHelpScreen(LineRenderer& lr, float w, float h, const Palette& pal) {
    Color ui = pal.ui;
    Color dim = withA(pal.ui, 0.65f);
    Color rim = pal.rim;

    float pw = w * 0.86f, ph = h * 0.92f;
    float px = (w - pw) * 0.5f, py = (h - ph) * 0.5f;

    lr.fillRect(px, py, pw, ph, rgba(2, 4, 8, 250));
    Vec2 b[5] = {{px, py}, {px + pw, py}, {px + pw, py + ph}, {px, py + ph}, {px, py}};
    lr.poly(b, 4, withA(rim, 0.7f), 1, false);

    float fs = ph * 0.032f;
    float cx = w * 0.5f;
    float y = py + ph * 0.05f;

    VecFont::draw(lr, "HOW TO PLAY", cx, y, fs * 1.45f, ui, VecFont::CENTER, 2);
    y += fs * 2.05f;

    // mission
    VecFont::draw(lr, "You pilot the NEURO-VECTOR fighter on the rim of a warp tunnel.",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.25f;
    VecFont::draw(lr, "Geometric invaders materialize deep in the tunnel and crawl",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.25f;
    VecFont::draw(lr, "toward the rim. Shoot them down before they reach your lane,",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.25f;
    VecFont::draw(lr, "or lose a ship. Survive all 100 sectors.",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.65f;

    // two columns: controls | bestiary
    float colL = px + pw * 0.07f;
    float colR = px + pw * 0.55f;
    float rowH = fs * 1.35f;

    VecFont::draw(lr, "CONTROLS", colL, y, fs, rim, VecFont::LEFT, 1);
    VecFont::draw(lr, "ENEMIES", colR + fs * 1.2f, y, fs, rim, VecFont::LEFT, 1);
    y += fs * 1.6f;

    struct Ctl { const char* key; const char* act; };
    static const Ctl ctls[] = {
        {"ARROWS", "A D / ROTATE DIAL"}, {"MOUSE X", "ROTARY DIAL"},
        {"SPACE", "FIRE"}, {"SHIFT", "SMART BOMB"},
        {"E", "RING JUMP (HYPER)"}, {"ESC", "PAUSE"},
        {"F1", "DIAGNOSTICS"}, {"H", "THIS SCREEN"},
    };
    struct Foe { const char* name; const char* tip; int kind; };
    static const Foe foes[] = {
        {"SEEKER", "HOMES TO YOUR LANE", 0},
        {"FLIPPER", "FLIPS LANES AT THE RIM", 1},
        {"SPLITTER", "SPLITS IN TWO WHEN HIT", 2},
        {"PULSAR", "ZAPS ITS LANE - DODGE", 4},
        {"MIRROR", "INVERTS YOUR CONTROLS", 5},
        {"WEAVER", "PHASES IN AND OUT", 6},
    };

    float gy = y;
    for (const Ctl& c : ctls) {
        VecFont::draw(lr, c.key, colL, gy, fs * 0.85f, ui, VecFont::LEFT, 0);
        VecFont::draw(lr, c.act, colL + fs * 6.4f, gy, fs * 0.85f, dim, VecFont::LEFT, 0);
        gy += rowH;
    }
    float ey = y;
    for (const Foe& f : foes) {
        Color ec = pal.enemy[f.kind];
        drawEnemyGlyph(lr, f.kind, {colR, ey - fs * 0.32f}, fs * 0.55f, ec, 0.f);
        VecFont::draw(lr, f.name, colR + fs * 1.2f, ey, fs * 0.85f, ec, VecFont::LEFT, 0);
        VecFont::draw(lr, f.tip, colR + fs * 7.2f, ey, fs * 0.85f, dim, VecFont::LEFT, 0);
        ey += rowH;
    }
    y = (gy > ey ? gy : ey) + fs * 0.7f;

    lr.line(px + pw * 0.07f, y - fs * 0.4f, px + pw * 0.93f, y - fs * 0.4f, withA(rim, 0.4f), 0);

    // scoring
    VecFont::draw(lr, "SCORING", cx, y, fs, rim, VecFont::CENTER, 1); y += fs * 1.5f;
    VecFont::draw(lr, "Kills build your multiplier up to x64 - taking a hit resets it.",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.3f;
    VecFont::draw(lr, "Near-rim kills score up to 3x. Smart bombs vaporize the screen.",
                  cx, y, fs * 0.85f, dim, VecFont::CENTER, 0); y += fs * 1.3f;
    char buf[128];
    snprintf(buf, sizeof buf, "Extra ships at 20k / 50k / 120k. Smart bombs at 30k / 80k / 150k.");
    VecFont::draw(lr, buf, cx, y, fs * 0.85f, dim, VecFont::CENTER, 0);

    // footer pinned to the panel bottom — the content above never reaches it
    VecFont::draw(lr, "ENTER / H / ESC - BACK", cx, py + ph - fs * 1.1f, fs * 0.85f, ui, VecFont::CENTER, 1);
}

} // namespace nv
