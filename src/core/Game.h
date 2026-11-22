// Neuro-Vector — top-level game orchestrator (states, fixed-timestep loop, CRT pipeline).
#pragma once
#include "Input.h"

// global-scope forward declarations so nv members bind to SDL's real types
struct SDL_Window;
struct SDL_Renderer;

namespace nv {

class Game {
public:
    int run(int argc, char** argv);
    // Headless capture mode (--shot): dummy video driver, simulated ticks, BMP out.
    int shot(const char* path, int level, int frames, bool safe);
    // Headless regression test: drive the menus and verify every option works.
    int selfTestMenu();

private:
    int initSDL(bool headless);
    void shutdownSDL();
    void tick(float dt);
    void render(float alpha);
    // latch edges from the per-frame poll into the tick's pending intents so a
    // press is never lost on a frame that runs no fixed tick (and never doubled)
    void latch(const Intents& f);
    // -1/0/+1 with auto-repeat: immediate on press, then repeats while held
    int menuDir(const Intents& in, float dt);
    void startGame();
    void demoStart(int idx);
    void drawSceneWorld(float alpha);
    void drawSceneHud(float alpha);
    void drawTitle(float alpha);
    void drawOverlays();

    SDL_Window* win_ = nullptr;
    SDL_Renderer* ren_ = nullptr;
    int w_ = 1280, h_ = 720;
    bool headless_ = false;

    class Input* input_ = nullptr;
    class AudioEngine* audio_ = nullptr;
    class SaveData* save_ = nullptr;
    class Fx* fx_ = nullptr;
    class CrtPass* crt_ = nullptr;
    class LineRenderer* lr_ = nullptr;
    class World* world_ = nullptr;
    class World* demo_ = nullptr;

    int state_ = 0; // State enum (kept int to avoid exposing in header)
    Intents intents_;   // pending intents: edges latch until a tick consumes them
    bool fireLatch_ = false; // fire edge held over frames that run no tick
    bool aiPlay_ = false;
    bool present_ = true;
    bool help_ = false; // HOW TO PLAY overlay
    int  menuSel_ = 0;  // main menu: 0 START, 1 SAFE MODE, 2 CRT FX, 3 HELP, 4 DIAGNOSTICS
    int  opSel_ = 0;    // options panel: 0 SAFE, 1 CRT, 2 MOUSE, 3 VOLUME, 4 RESET
    float menuRepeatT_ = 0; // menu up/down auto-repeat timer
    float attractT_ = 0, gameoverT_ = 0, victoryT_ = 0, blinkT_ = 0;
    float gameClock_ = 0, acc_ = 0;
    bool attractDemo_ = false;
    int demoIdx_ = 0, loop_ = 0;
    bool operatorMenu_ = false, quit_ = false;
    char initials_[4] = {'A', 'A', 'A', 0};
    int iniSlot_ = 0, iniLetter_ = 0;
    float iniRepeatT_ = 0;
};

} // namespace nv
