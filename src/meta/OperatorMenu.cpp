#include "OperatorMenu.h"
#include "../renderer/LineRenderer.h"
#include "../renderer/VecFont.h"
#include "../game/RenderWorld.h"
#include "../core/Config.h"
#include <cmath>
#include <cstdio>

namespace nv {

void drawOperatorMenu(LineRenderer& lr, float w, float h, float t, const Settings& s,
                      int scoreCount, int sel) {
    Color grn = rgba(120, 255, 140, 230);
    Color grnDim = rgba(80, 200, 100, 160);
    Color grnDark = rgba(40, 120, 55, 120);

    float pw = w * 0.62f, ph = h * 0.78f;
    float px = (w - pw) * 0.5f, py = (h - ph) * 0.5f;

    lr.fillRect(px, py, pw, ph, rgba(4, 10, 6, 250));
    Vec2 c[5] = {{px, py}, {px + pw, py}, {px + pw, py + ph}, {px, py + ph}, {px, py}};
    lr.poly(c, 4, grnDim, 1, false);

    float x = px + pw * 0.06f;
    float y = py + ph * 0.09f;
    float fs = ph * 0.042f;
    char buf[128];

    VecFont::draw(lr, "NEURO-VECTOR DIAGNOSTICS v1.4", x, y, fs * 1.15f, grn, VecFont::LEFT, 1);
    y += fs * 2.1f;

    snprintf(buf, sizeof buf, "PULSE-STRESS:      %04d", (int)(500 + 480 * std::sin(t * 2.7)));
    VecFont::draw(lr, buf, x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.5f;
    snprintf(buf, sizeof buf, "ADRENAL-BASIS:     %04d", (int)(500 + 480 * std::sin(t * 1.9 + 1.3)));
    VecFont::draw(lr, buf, x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.5f;
    snprintf(buf, sizeof buf, "SENSE-WIPE:        %04d", (int)(500 + 480 * std::sin(t * 3.4 + 0.7)));
    VecFont::draw(lr, buf, x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.5f;
    snprintf(buf, sizeof buf, "BIOMETRIC HARVEST: %d%%", 40 + (int)(58 * (0.5 + 0.5 * std::sin(t * 0.8))));
    VecFont::draw(lr, buf, x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.5f;
    VecFont::draw(lr, "DATA-HARVEST SYSTEM: ACTIVE", x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.5f;
    VecFont::draw(lr, "MIRROR-TEST:       PASSED", x, y, fs, grnDim, VecFont::LEFT, 0); y += fs * 1.9f;

    lr.line(x, y, px + pw * 0.94f, y, grnDark, 0); y += fs * 1.1f;

    // selectable option rows (up/down + ENTER)
    float rowY[5];
    float rowH = fs * 1.5f;
    snprintf(buf, sizeof buf, "SAFE MODE:          %s", s.safeMode ? "ON" : "OFF");
    VecFont::draw(lr, buf, x + fs * 1.6f, y, fs, grn, VecFont::LEFT, 0);
    rowY[0] = y; y += rowH;
    snprintf(buf, sizeof buf, "CRT FX:             %s", s.crtFx ? "ON" : "OFF");
    VecFont::draw(lr, buf, x + fs * 1.6f, y, fs, grn, VecFont::LEFT, 0);
    rowY[1] = y; y += rowH;
    snprintf(buf, sizeof buf, "MOUSE DIAL:         %s", s.mouseDial ? "ON" : "OFF");
    VecFont::draw(lr, buf, x + fs * 1.6f, y, fs, grn, VecFont::LEFT, 0);
    rowY[2] = y; y += rowH;
    snprintf(buf, sizeof buf, "MASTER VOLUME:      %d%%", (int)(s.volMaster * 100));
    VecFont::draw(lr, buf, x + fs * 1.6f, y, fs, grn, VecFont::LEFT, 0);
    rowY[3] = y; y += rowH;
    snprintf(buf, sizeof buf, "RESET HIGH SCORES   (%d)", scoreCount);
    VecFont::draw(lr, buf, x + fs * 1.6f, y, fs, grn, VecFont::LEFT, 0);
    rowY[4] = y; y += rowH;

    // cursor: a small ship next to the selected row
    if (sel >= 0 && sel < 5)
        drawShipIcon(lr, {x + fs * 0.45f, rowY[sel] - fs * 0.32f}, fs * 0.5f, grn);

    y += fs * 0.9f;
    VecFont::draw(lr, "UP/DOWN: MOVE   ENTER: SELECT", x, y, fs * 0.85f, grnDim, VecFont::LEFT, 0);
    VecFont::draw(lr, "[ESC/F1] BACK", x, py + ph - fs * 1.6f, fs * 0.9f, grnDim, VecFont::LEFT, 0);
}

} // namespace nv
