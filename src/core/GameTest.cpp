// Neuro-Vector — headless menu regression test.
// Drives the real Game::tick() state machine with synthetic input and verifies
// the fused single main screen: every menu option works with ENTER and the
// up/down navigation (with key auto-repeat) responds on every press.
#include "Game.h"
#include "Config.h"
#include "Common.h"
#include "../audio/AudioEngine.h"
#include "../fx/Fx.h"
#include "../game/World.h"
#include <cstdio>

namespace nv {

// must match the file-local State enum in Game.cpp (state_ is an int in Game.h)
enum State { WARNING = 0, ATTRACT = 1, PLAY = 2, PAUSE = 3, GAMEOVER = 4, INITIALS = 5, VICTORY = 6 };

int Game::selfTestMenu() {
    headless_ = true;
    save_ = new SaveData();       // defaults, no disk access
    audio_ = new AudioEngine();   // no device: play() is a harmless ring write
    fx_ = new Fx();
    world_ = new World();
    demo_ = new World();
    state_ = ATTRACT;
    attractDemo_ = false;
    menuSel_ = 0;
    help_ = false;
    operatorMenu_ = false;
    opSel_ = 0;
    int fails = 0;
    const float step = 1.f / 60.f;

    auto tick1 = [&](bool (Intents::*set)) {
        Intents in{};
        in.*set = true;
        intents_ = in;
        tick(step);
    };
    auto tickHeld = [&](int ticks) { // hold menuDown for N ticks (auto-repeat)
        Intents in{};
        in.menuDownHeld = true;
        for (int i = 0; i < ticks; i++) {
            intents_ = in;
            tick(step);
        }
    };

    // main menu: 0 START, 1 SAFE MODE, 2 CRT FX, 3 HELP, 4 DIAGNOSTICS

    // 1. DOWN moves to SAFE MODE, ENTER toggles it, stays on the main screen
    tick1(&Intents::menuDown);
    if (menuSel_ != 1) { printf("FAIL menu: DOWN navigation (sel=%d)\n", menuSel_); fails++; }
    bool before = save_->settings.safeMode;
    tick1(&Intents::startPressed);
    if (save_->settings.safeMode == before) { printf("FAIL menu: SAFE MODE toggle on ENTER\n"); fails++; }
    if (help_ || operatorMenu_ || state_ != ATTRACT) { printf("FAIL menu: SAFE MODE left the main screen\n"); fails++; }

    // 2. DOWN to CRT FX, ENTER toggles it
    tick1(&Intents::menuDown);
    if (menuSel_ != 2) { printf("FAIL menu: DOWN navigation to CRT FX (sel=%d)\n", menuSel_); fails++; }
    before = save_->settings.crtFx;
    tick1(&Intents::startPressed);
    if (save_->settings.crtFx == before) { printf("FAIL menu: CRT FX toggle on ENTER\n"); fails++; }

    // 3. DOWN to HOW TO PLAY, ENTER opens the help; ENTER closes it
    tick1(&Intents::menuDown);
    if (menuSel_ != 3) { printf("FAIL menu: DOWN navigation to HELP (sel=%d)\n", menuSel_); fails++; }
    tick1(&Intents::startPressed);
    if (!help_) { printf("FAIL menu: HOW TO PLAY did not open on ENTER\n"); fails++; }
    tick1(&Intents::startPressed);
    if (help_) { printf("FAIL menu: help did not close on ENTER\n"); fails++; }

    // 4. DOWN to DIAGNOSTICS, ENTER opens the panel, ENTER toggles a row, ESC closes
    tick1(&Intents::menuDown);
    if (menuSel_ != 4) { printf("FAIL menu: DOWN navigation to DIAGNOSTICS (sel=%d)\n", menuSel_); fails++; }
    tick1(&Intents::startPressed);
    if (!operatorMenu_) { printf("FAIL menu: DIAGNOSTICS did not open on ENTER\n"); fails++; }
    before = save_->settings.safeMode;
    tick1(&Intents::startPressed);
    if (save_->settings.safeMode == before) { printf("FAIL menu: panel row toggle on ENTER\n"); fails++; }
    if (!operatorMenu_) { printf("FAIL menu: panel closed after toggle\n"); fails++; }
    tick1(&Intents::pausePressed);
    if (operatorMenu_) { printf("FAIL menu: panel did not close on ESC\n"); fails++; }

    // 5. UP x4 back to START GAME, ENTER starts the game
    tick1(&Intents::menuUp);
    tick1(&Intents::menuUp);
    tick1(&Intents::menuUp);
    tick1(&Intents::menuUp);
    if (menuSel_ != 0) { printf("FAIL menu: UP navigation back to START (sel=%d)\n", menuSel_); fails++; }
    tick1(&Intents::startPressed);
    if (state_ != PLAY) { printf("FAIL menu: START GAME did not start on ENTER\n"); fails++; }

    // 6. held DOWN auto-repeats (regression: menu keys "not working / slow")
    state_ = ATTRACT;
    attractDemo_ = false;
    menuSel_ = 0;
    menuRepeatT_ = 0;
    tickHeld(45); // 0.75 s: initial delay 0.42 s + repeats
    if (menuSel_ == 0) { printf("FAIL menu: held DOWN does not auto-repeat (sel=%d)\n", menuSel_); fails++; }
    if (menuSel_ < 0 || menuSel_ > 4) { printf("FAIL menu: repeat wrapped out of range (sel=%d)\n", menuSel_); fails++; }

    // 7. a fresh press always moves exactly one step
    state_ = ATTRACT;
    menuSel_ = 0;
    menuRepeatT_ = 0;
    tick1(&Intents::menuDown);
    if (menuSel_ != 1) { printf("FAIL menu: fresh DOWN press did not move (sel=%d)\n", menuSel_); fails++; }
    intents_ = Intents{}; // release: no further movement
    tick(step);
    if (menuSel_ != 1) { printf("FAIL menu: selection moved without input (sel=%d)\n", menuSel_); fails++; }

    // 8. real input path: FIRE held via latch() spawns bullets in PLAY
    state_ = PLAY;
    world_->startLevel(0, 0, false);
    world_->phase = WorldPhase::ACTIVE; // skip the intro card
    intents_ = Intents{};
    {
        Intents held{}; // held fire, latched per frame like the run loop does
        held.fire = true;
        for (int i = 0; i < 40; i++) { latch(held); tick(step); }
    }
    if (world_->bullets.empty()) { printf("FAIL fire: holding FIRE spawns no bullets\n"); fails++; }
    if (world_->pl.fireCd < 0.f) { printf("FAIL fire: fire cooldown desynced\n"); fails++; }

    // 9. input layer: SPACE keydown registers, mouse click fires
    {
        Input input;
        SDL_Event e{}; // space tap: the edge must register
        e.type = SDL_KEYDOWN;
        e.key.keysym.sym = SDLK_SPACE;
        input.handleEvent(e);
        Intents in = input.poll(false);
        if (!in.firePressed) { printf("FAIL fire: SPACE keydown edge missing\n"); fails++; }
        SDL_Event m{}; // left click: must fire
        m.type = SDL_MOUSEBUTTONDOWN;
        m.button.button = SDL_BUTTON_LEFT;
        input.handleEvent(m);
        in = input.poll(false);
        if (!in.fire) { printf("FAIL fire: mouse click does not fire\n"); fails++; }
    }

    // 10. ring jump: E flips tracks on hyper sectors, denied with feedback below 67
    world_->startLevel(79, 0, false); // hyper sector: dual rings
    world_->phase = WorldPhase::ACTIVE;
    intents_ = Intents{};
    {
        Intents f{};
        f.ringPressed = true;
        latch(f);
        tick(step);
    }
    if (world_->pl.track != 1) { printf("FAIL ring: E does not jump on hyper sectors\n"); fails++; }
    world_->startLevel(5, 0, false); // early sector: single ring
    world_->phase = WorldPhase::ACTIVE;
    intents_ = Intents{};
    {
        Intents f{};
        f.ringPressed = true;
        latch(f);
        tick(step);
    }
    if (world_->pl.track != 0) { printf("FAIL ring: track flipped on a single-ring sector\n"); fails++; }
    if (world_->noRingT <= 0.f) { printf("FAIL ring: no feedback when ring jump is unavailable\n"); fails++; }

    delete save_; delete audio_; delete fx_; delete world_; delete demo_;
    save_ = nullptr; audio_ = nullptr; fx_ = nullptr; world_ = nullptr; demo_ = nullptr;

    if (!fails) printf("menu+fire+ring: main screen ok, ENTER works, up/down repeats, ship fires, ring jump ok\n");
    return fails;
}

} // namespace nv
