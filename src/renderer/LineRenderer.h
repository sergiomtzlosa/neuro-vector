// Neuro-Vector — line renderer with phosphor glow (multi-pass additive strokes).
#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "../core/Common.h"
#include <vector>

namespace nv {

class LineRenderer {
public:
    void begin(SDL_Renderer* r) { r_ = r; }

    // glow: 0 = single pass, 1 = soft bloom, 2 = hot (player/score moments)
    void line(float x1, float y1, float x2, float y2, Color c, int glow = 0) {
        if (glow <= 0) {
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
            SDL_RenderDrawLine(r_, (int)x1, (int)y1, (int)x2, (int)y2);
            return;
        }
        float dx = x2 - x1, dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        float nx = 0, ny = 0;
        if (len > 0.001f) { nx = -dy / len; ny = dx / len; }
        int layers = glow >= 2 ? 3 : 2;
        float spread = glow >= 2 ? 2.0f : 1.2f;
        for (int i = layers; i >= 1; i--) {
            float w = spread * (float)i;
            int a = (int)(c.a * (glow >= 2 ? 0.30f : 0.22f) / (float)i);
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, (uint8_t)a);
            SDL_RenderDrawLine(r_, (int)(x1 + nx * w), (int)(y1 + ny * w), (int)(x2 + nx * w), (int)(y2 + ny * w));
            SDL_RenderDrawLine(r_, (int)(x1 - nx * w), (int)(y1 - ny * w), (int)(x2 - nx * w), (int)(y2 - ny * w));
        }
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
        SDL_RenderDrawLine(r_, (int)x1, (int)y1, (int)x2, (int)y2);
    }

    void poly(const Vec2* pts, int n, Color c, int glow = 0, bool closed = true) {
        for (int i = 0; i < n - 1; i++)
            line(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, c, glow);
        if (closed && n > 2)
            line(pts[n - 1].x, pts[n - 1].y, pts[0].x, pts[0].y, c, glow);
    }

    void fillRect(float x, float y, float w, float h, Color c) {
        SDL_Rect rc{(int)x, (int)y, (int)w, (int)h};
        SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(r_, &rc);
    }

private:
    SDL_Renderer* r_ = nullptr;
};

} // namespace nv
