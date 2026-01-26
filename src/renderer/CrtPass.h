// Neuro-Vector — CRT post-processing (pure SDL2 path).
//
// Pipeline per frame:
//   1. world lines -> accumW target  (newAccum = prev*decay + world, MOD+ADD)
//   2. HUD lines   -> accumH target  (short decay = burn-in ghosting)
//   3. present: barrel-warp mesh draw of accum (+HUD additive)
//      -> additive bloom copies -> chromatic aberration (channel split)
//      -> scanlines -> vignette -> flash overlay
// All effects toggleable; Safe Mode shortens persistence and softens flashes.
#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <vector>

namespace nv {

class CrtPass {
public:
    void resize(SDL_Renderer* r, int w, int h);
    void shutdown();

    // Phosphor accumulators. Call beginWorld, draw world lines (additive),
    // then beginHud, draw HUD lines, then present().
    SDL_Texture* beginWorld(SDL_Renderer* r, float decay);
    SDL_Texture* beginHud(SDL_Renderer* r, float decay);
    void present(SDL_Renderer* r, float aberrPx, bool fxEnabled, float safeFade);
    void drawFlash(SDL_Renderer* r, float intensity);

    int width() const { return w_; }
    int height() const { return h_; }
    void debugDump(SDL_Renderer* r, const char* path);

private:
    void clearToDecay(SDL_Renderer* r, SDL_Texture* target, SDL_Texture* prev, float decay);
    void buildWarpMesh(int w, int h);

    int w_ = 0, h_ = 0;
    bool primed_ = false; // accumulators hold valid data after the first present
    SDL_Texture* accumW_[2] = {nullptr, nullptr};
    SDL_Texture* accumH_[2] = {nullptr, nullptr};
    int ping_ = 0;
    SDL_Texture* scanline_ = nullptr;
    SDL_Texture* vignette_ = nullptr;
    std::vector<SDL_Vertex> warp_;
    std::vector<int> warpIdx_;
};

} // namespace nv
