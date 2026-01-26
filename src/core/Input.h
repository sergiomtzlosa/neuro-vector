// Neuro-Vector — input: keyboard, gamepad, mouse dial.
#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace nv {

struct Intents {
    float rotAxis = 0;        // -1..1 continuous rotation
    float mouseDX = 0;        // accumulated mouse delta this frame (dial)
    bool  fire = false;       // held
    bool  superPressed = false;
    bool  ringPressed = false;
    bool  startPressed = false;
    bool  pausePressed = false;
    bool  operatorPressed = false;
    bool  fullscreenPressed = false;
    bool  helpPressed = false;      // H: how-to-play screen
    bool  firePressed = false;      // fire edge (menus)
    bool  menuUp = false;           // menu navigation (fresh press)
    bool  menuDown = false;
    bool  menuUpHeld = false;       // menu navigation held (auto-repeat)
    bool  menuDownHeld = false;
    bool  anyPressed = false;       // generic "press something"
    int   digitPressed = -1;  // 0..9 for operator menu
};

class Input {
public:
    void init();
    void shutdown();
    void handleEvent(const union SDL_Event& e);
    Intents poll(bool mouseDial);
    void endFrame();
    bool quit = false;

private:
    const unsigned char* keys_ = nullptr;
    void* gc_ = nullptr; // SDL_GameController*
    float mouseAccum_ = 0;
    // edges captured from events
    bool  evSuper_ = false, evRing_ = false, evStart_ = false, evPause_ = false;
    bool  evOperator_ = false, evFullscreen_ = false, evAny_ = false;
    bool  evHelp_ = false;
    bool  evFire_ = false;
    bool  evMenuUp_ = false, evMenuDown_ = false;
    bool  gcFire_ = false;
    bool  gcDpadUp_ = false, gcDpadDown_ = false;
    bool  mouseFire_ = false;
    int   evDigit_ = -1;
};

} // namespace nv
