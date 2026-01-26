#include "CrtPass.h"
#include <cmath>

namespace nv {

void CrtPass::resize(SDL_Renderer* r, int w, int h) {
    shutdown();
    w_ = w; h_ = h;

    SDL_Texture** accums[4] = {&accumW_[0], &accumW_[1], &accumH_[0], &accumH_[1]};
    for (auto t : accums) {
        *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
        SDL_SetTextureBlendMode(*t, SDL_BLENDMODE_BLEND);
    }

    // scanline texture: 1 x h column (stretched horizontally), per-row pattern.
    // NOTE: pitch must be width * 4 bytes = 4 here; a bad pitch reads out-of-bounds
    // rows which render as black under MOD blend (the "black square" bug).
    scanline_ = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 1, h);
    if (scanline_) {
        Uint32* buf = new Uint32[h];
        for (int y = 0; y < h; y++) {
            Uint8 v = (y % 4 == 3) ? 120 : 255;
            buf[y] = 0xFF000000u | ((Uint32)v << 16) | ((Uint32)v << 8) | v;
        }
        SDL_UpdateTexture(scanline_, nullptr, buf, 1 * sizeof(Uint32));
        SDL_SetTextureBlendMode(scanline_, SDL_BLENDMODE_MOD);
        delete[] buf;
    }

    // vignette: full-screen radial falloff, multiply blend
    vignette_ = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (vignette_) {
        Uint32* buf = new Uint32[w * h];
        float cx = w * 0.5f, cy = h * 0.5f;
        float rmax = std::sqrt(cx * cx + cy * cy);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float dx = x - cx, dy = y - cy;
                float rr = std::sqrt(dx * dx + dy * dy) / rmax;
                float v = 1.f - 0.45f * rr * rr;
                Uint8 g = (Uint8)(v * 255.f);
                buf[y * w + x] = 0xFF000000u | ((Uint32)g << 16) | ((Uint32)g << 8) | g;
            }
        }
        SDL_UpdateTexture(vignette_, nullptr, buf, w * sizeof(Uint32));
        SDL_SetTextureBlendMode(vignette_, SDL_BLENDMODE_MOD);
        delete[] buf;
    }

    buildWarpMesh(w, h);
}

void CrtPass::shutdown() {
    SDL_Texture** accums[4] = {&accumW_[0], &accumW_[1], &accumH_[0], &accumH_[1]};
    for (auto t : accums) { if (*t) SDL_DestroyTexture(*t); *t = nullptr; }
    if (scanline_) { SDL_DestroyTexture(scanline_); scanline_ = nullptr; }
    if (vignette_) { SDL_DestroyTexture(vignette_); vignette_ = nullptr; }
    warp_.clear(); warpIdx_.clear();
    primed_ = false;
}

void CrtPass::buildWarpMesh(int w, int h) {
    const int GW = 24, GH = 14;
    warp_.clear(); warpIdx_.clear();
    float cx = w * 0.5f, cy = h * 0.5f;
    float rmax = std::sqrt(cx * cx + cy * cy);
    for (int gy = 0; gy <= GH; gy++) {
        for (int gx = 0; gx <= GW; gx++) {
            float u = (float)gx / GW, v = (float)gy / GH;
            float px = u * w, py = v * h;
            float dx = px - cx, dy = py - cy;
            float r2 = (dx * dx + dy * dy) / (rmax * rmax);
            float k = 1.f + 0.11f * r2; // barrel bulge
            SDL_Vertex vt;
            vt.position.x = cx + dx * k;
            vt.position.y = cy + dy * k;
            vt.tex_coord.x = u;
            vt.tex_coord.y = v;
            vt.color = SDL_Color{255, 255, 255, 255};
            warp_.push_back(vt);
        }
    }
    for (int gy = 0; gy < GH; gy++) {
        for (int gx = 0; gx < GW; gx++) {
            int i0 = gy * (GW + 1) + gx;
            int i1 = i0 + 1, i2 = i0 + (GW + 1), i3 = i2 + 1;
            warpIdx_.push_back(i0); warpIdx_.push_back(i1); warpIdx_.push_back(i2);
            warpIdx_.push_back(i1); warpIdx_.push_back(i3); warpIdx_.push_back(i2);
        }
    }
}

void CrtPass::clearToDecay(SDL_Renderer* r, SDL_Texture* target, SDL_Texture* prev, float decay) {
    SDL_SetRenderTarget(r, target);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    // first render (no phosphor history): clear to black; afterwards clear to
    // the decay gray so the MOD copy scales the previous frame down
    Uint8 d = primed_ ? (Uint8)(decay * 255.f) : 0;
    SDL_SetRenderDrawColor(r, d, d, d, 255);
    SDL_RenderClear(r); // dst = const decay gray (or black before any history)
    if (prev && primed_) {
        SDL_SetTextureBlendMode(prev, SDL_BLENDMODE_MOD);
        SDL_SetTextureColorMod(prev, 255, 255, 255);
        SDL_RenderCopy(r, prev, nullptr, nullptr); // dst = decay * prev
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD); // world lines add on top
}

SDL_Texture* CrtPass::beginWorld(SDL_Renderer* r, float decay) {
    ping_ ^= 1;
    clearToDecay(r, accumW_[ping_], accumW_[ping_ ^ 1], decay);
    return accumW_[ping_];
}

SDL_Texture* CrtPass::beginHud(SDL_Renderer* r, float decay) {
    clearToDecay(r, accumH_[ping_], accumH_[ping_ ^ 1], decay);
    return accumH_[ping_];
}

void CrtPass::present(SDL_Renderer* r, float aberrPx, bool fxEnabled, float safeFade) {
    (void)safeFade;
    primed_ = true;
    SDL_SetRenderTarget(r, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    if (!fxEnabled) return; // caller drew straight to backbuffer

    SDL_Rect full{0, 0, w_, h_};
    if (aberrPx > 0.5f) {
        // chromatic aberration: additive R/G/B channel splits reconstruct color
        struct Ch { int dx; SDL_Color c; };
        Ch chans[3] = {{(int)aberrPx, SDL_Color{255, 40, 40, 255}},
                       {0,            SDL_Color{40, 255, 40, 255}},
                       {-(int)aberrPx, SDL_Color{60, 60, 255, 255}}};
        SDL_SetTextureBlendMode(accumW_[ping_], SDL_BLENDMODE_ADD);
        SDL_SetTextureBlendMode(accumH_[ping_], SDL_BLENDMODE_ADD);
        for (Ch& c : chans) {
            SDL_SetTextureColorMod(accumW_[ping_], c.c.r, c.c.g, c.c.b);
            SDL_Rect rc{c.dx, 0, w_, h_};
            SDL_RenderCopy(r, accumW_[ping_], nullptr, &rc);
            SDL_SetTextureColorMod(accumH_[ping_], c.c.r, c.c.g, c.c.b);
            SDL_RenderCopy(r, accumH_[ping_], nullptr, &rc);
        }
        SDL_SetTextureColorMod(accumW_[ping_], 255, 255, 255);
        SDL_SetTextureColorMod(accumH_[ping_], 255, 255, 255);
    } else {
        // barrel-warp mesh draw
        SDL_SetTextureBlendMode(accumW_[ping_], SDL_BLENDMODE_NONE);
        SDL_RenderGeometry(r, accumW_[ping_], warp_.data(), (int)warp_.size(),
                           warpIdx_.data(), (int)warpIdx_.size());
        SDL_SetTextureBlendMode(accumH_[ping_], SDL_BLENDMODE_ADD);
        SDL_RenderGeometry(r, accumH_[ping_], warp_.data(), (int)warp_.size(),
                           warpIdx_.data(), (int)warpIdx_.size());
    }

    // additive bloom: two slightly scaled copies
    SDL_SetTextureBlendMode(accumW_[ping_], SDL_BLENDMODE_ADD);
    struct Bloom { float infl; Uint8 a; };
    Bloom blooms[2] = {{0.016f, 80}, {0.036f, 45}};
    for (Bloom& b : blooms) {
        SDL_SetTextureAlphaMod(accumW_[ping_], b.a);
        SDL_Rect rc{(int)(-w_ * b.infl * 0.5f), (int)(-h_ * b.infl * 0.5f),
                    (int)(w_ * (1 + b.infl)), (int)(h_ * (1 + b.infl))};
        SDL_RenderCopy(r, accumW_[ping_], nullptr, &rc);
    }
    SDL_SetTextureAlphaMod(accumW_[ping_], 255);

    // scanlines + vignette (multiply) — env toggles for debugging
    if (scanline_ && !SDL_getenv("NV_NOSCAN")) SDL_RenderCopy(r, scanline_, nullptr, &full);
    if (vignette_ && !SDL_getenv("NV_NOVIG")) SDL_RenderCopy(r, vignette_, nullptr, &full);
}

// debug: dump the world accumulator to a BMP
void CrtPass::debugDump(SDL_Renderer* r, const char* path) {
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w_, h_, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s) {
        SDL_SetRenderTarget(r, accumW_[ping_]);
        SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
        printf("accum dump: %s\n", path);
    }
    SDL_SetRenderTarget(r, nullptr);
}

void CrtPass::drawFlash(SDL_Renderer* r, float intensity) {
    if (intensity <= 0.01f) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
    Uint8 a = (Uint8)(intensity * 200.f);
    SDL_SetRenderDrawColor(r, 255, 255, 255, a);
    SDL_Rect full{0, 0, w_, h_};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
}

} // namespace nv
