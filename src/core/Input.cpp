#include "Input.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace nv {

void Input::init() {
    keys_ = SDL_GetKeyboardState(nullptr);
    if (SDL_IsGameController(0))
        gc_ = SDL_GameControllerOpen(0);
}

void Input::shutdown() {
    if (gc_) SDL_GameControllerClose((SDL_GameController*)gc_);
    gc_ = nullptr;
}

void Input::handleEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT:
        quit = true;
        break;
    case SDL_KEYDOWN:
        if (e.key.repeat) break;
        evAny_ = true;
        switch (e.key.keysym.sym) {
        case SDLK_LSHIFT: case SDLK_RSHIFT: evSuper_ = true; break;
        case SDLK_e: evRing_ = true; break;
        case SDLK_SPACE: evFire_ = true; break;
        case SDLK_RETURN: case SDLK_c: evStart_ = true; break;
        case SDLK_ESCAPE: evPause_ = true; break;
        case SDLK_F1: evOperator_ = true; break;
        case SDLK_h: evHelp_ = true; break;
        case SDLK_UP: case SDLK_w: evMenuUp_ = true; break;
        case SDLK_DOWN: case SDLK_s: evMenuDown_ = true; break;
        case SDLK_F11: evFullscreen_ = true; break;
        default:
            if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9)
                evDigit_ = e.key.keysym.sym - SDLK_0;
            break;
        }
        break;
    case SDL_MOUSEMOTION:
        mouseAccum_ += (float)e.motion.xrel;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT) { mouseFire_ = true; evFire_ = true; }
        break;
    case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT) mouseFire_ = false;
        break;
    case SDL_CONTROLLERDEVICEADDED:
        if (!gc_ && SDL_IsGameController(e.cdevice.which))
            gc_ = SDL_GameControllerOpen(e.cdevice.which);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        evAny_ = true;
        switch (e.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: gcFire_ = true; evFire_ = true; break;
        case SDL_CONTROLLER_BUTTON_B: evSuper_ = true; break;
        case SDL_CONTROLLER_BUTTON_Y: evRing_ = true; break;
        case SDL_CONTROLLER_BUTTON_START: evStart_ = true; evPause_ = true; break;
        case SDL_CONTROLLER_BUTTON_BACK: evOperator_ = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: evMenuUp_ = true; gcDpadUp_ = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: evMenuDown_ = true; gcDpadDown_ = true; break;
        }
        break;
    case SDL_CONTROLLERBUTTONUP:
        switch (e.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: gcFire_ = false; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: gcDpadUp_ = false; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: gcDpadDown_ = false; break;
        }
        break;
    default:
        break;
    }
}

Intents Input::poll(bool mouseDial) {
    Intents in;
    // rotation: keyboard
    if (keys_) {
        bool left  = keys_[SDL_SCANCODE_LEFT] || keys_[SDL_SCANCODE_A];
        bool right = keys_[SDL_SCANCODE_RIGHT] || keys_[SDL_SCANCODE_D];
        in.rotAxis = (right ? 1.f : 0.f) - (left ? 1.f : 0.f);
    }
    // gamepad stick overrides keys when pushed
    if (gc_) {
        float ax = SDL_GameControllerGetAxis((SDL_GameController*)gc_, SDL_CONTROLLER_AXIS_LEFTX) / 32767.f;
        if (ax > 0.25f || ax < -0.25f) in.rotAxis = ax;
        else {
            SDL_GameControllerButton b = SDL_GameControllerGetButton((SDL_GameController*)gc_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? SDL_CONTROLLER_BUTTON_DPAD_RIGHT
                                       : SDL_GameControllerGetButton((SDL_GameController*)gc_, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? SDL_CONTROLLER_BUTTON_DPAD_LEFT
                                       : SDL_CONTROLLER_BUTTON_INVALID;
            if (b == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) in.rotAxis = 1;
            else if (b == SDL_CONTROLLER_BUTTON_DPAD_LEFT) in.rotAxis = -1;
        }
    }
    in.fire = (keys_ && keys_[SDL_SCANCODE_SPACE]) || gcFire_ || mouseFire_ || evFire_;
    in.firePressed = evFire_;
    in.mouseDX = mouseDial ? mouseAccum_ : 0;
    in.superPressed = evSuper_;
    in.ringPressed = evRing_;
    in.startPressed = evStart_;
    in.pausePressed = evPause_;
    in.operatorPressed = evOperator_;
    in.fullscreenPressed = evFullscreen_;
    in.helpPressed = evHelp_;
    in.menuUp = evMenuUp_;
    in.menuDown = evMenuDown_;
    in.menuUpHeld = (keys_ && (keys_[SDL_SCANCODE_UP] || keys_[SDL_SCANCODE_W])) || gcDpadUp_;
    in.menuDownHeld = (keys_ && (keys_[SDL_SCANCODE_DOWN] || keys_[SDL_SCANCODE_S])) || gcDpadDown_;
    in.anyPressed = evAny_ || in.startPressed || in.fire;
    in.digitPressed = evDigit_;
    return in;
}

void Input::endFrame() {
    mouseAccum_ = 0;
    evSuper_ = evRing_ = evStart_ = evPause_ = evOperator_ = evFullscreen_ = evAny_ = false;
    evHelp_ = evMenuUp_ = evMenuDown_ = evFire_ = false;
    evDigit_ = -1;
}

} // namespace nv
