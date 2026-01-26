#include "Game.h"
#include "Input.h"
#include "Config.h"
#include "Common.h"
#include "../audio/AudioEngine.h"
#include "../fx/Fx.h"
#include "../renderer/CrtPass.h"
#include "../renderer/LineRenderer.h"
#include "../renderer/VecFont.h"
#include "../game/World.h"
#include "../game/RenderWorld.h"
#include "../meta/OperatorMenu.h"
#include "../meta/HelpScreen.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cmath>
#include <cstdio>

namespace nv {

enum State { WARNING = 0, ATTRACT = 1, PLAY = 2, PAUSE = 3, GAMEOVER = 4, INITIALS = 5, VICTORY = 6 };
static const int kDemoLevels[4] = {1, 12, 40, 78};
static const char* kPhrases[6] = {
    "FORGET", "OBEY", "SENSE-WIPE",
    "FORGET EVERYTHING", "SENSES ERASED", "NO ESCAPE"
};

int Game::initSDL(bool headless) {
    headless_ = headless;
    if (headless)
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | (headless ? 0 : SDL_INIT_AUDIO)) < 0) {
        // retry video-only (e.g. no audio device / gamepad subsystem)
        SDL_Quit();
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
    }
    return 0;
}

void Game::shutdownSDL() {
    if (audio_) audio_->shutdown();
    if (input_) input_->shutdown();
    if (crt_) crt_->shutdown();
    if (ren_) SDL_DestroyRenderer(ren_);
    if (win_) SDL_DestroyWindow(win_);
    SDL_Quit();
}

int Game::run(int argc, char** argv) {
    (void)argc; (void)argv;
    if (initSDL(false)) return 1;

    save_ = new SaveData();
    save_->load();

    Uint32 flags = SDL_WINDOW_RESIZABLE;
    if (save_->settings.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    win_ = SDL_CreateWindow("NEURO-VECTOR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            w_, h_, flags);
    if (!win_) { fprintf(stderr, "window: %s\n", SDL_GetError()); shutdownSDL(); return 1; }
    ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren_)
        ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_SOFTWARE);
    if (!ren_) { fprintf(stderr, "renderer: %s\n", SDL_GetError()); shutdownSDL(); return 1; }
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_ADD);
    SDL_GetRendererOutputSize(ren_, &w_, &h_);

    VecFont::init();
    crt_ = new CrtPass();
    crt_->resize(ren_, w_, h_);
    lr_ = new LineRenderer();
    input_ = new Input();
    input_->init();
    audio_ = new AudioEngine();
    if (!audio_->init())
        fprintf(stderr, "audio unavailable, running silent\n");
    audio_->setVolumes(save_->settings.volMaster, save_->settings.volMusic,
                       save_->settings.volSfx, save_->settings.volDrone);
    fx_ = new Fx();
    world_ = new World();
    demo_ = new World();

    world_->startGame(0, 0);
    demoStart(0);
    fx_->notifyLevel(0, world_->P.hash);
    state_ = ATTRACT; // single main screen: title + warning + options menu
    audio_->setMusic(0, 0.15f);

    Uint64 pc = SDL_GetPerformanceCounter();
    const float step = 1.f / 60.f;
    while (!quit_) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            input_->handleEvent(e);
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 e.window.event == SDL_WINDOWEVENT_RESIZED)) {
                SDL_GetRendererOutputSize(ren_, &w_, &h_);
                crt_->resize(ren_, w_, h_);
            }
        }
        if (input_->quit) break;
        // self-driving: NV_DEMO_KEYS=1 injects ENTER then SPACE taps as real
        // SDL events — end-to-end test of the event-pump -> fire chain
        if (SDL_getenv("NV_DEMO_KEYS")) {
            static Uint64 next = 0;
            static bool started = false;
            Uint64 now = SDL_GetTicks();
            if (now > next) {
                next = now + (started ? 300 : 1200);
                SDL_Event d{}, u{};
                d.type = SDL_KEYDOWN;
                d.key.keysym.sym = started ? SDLK_SPACE : SDLK_RETURN;
                u.type = SDL_KEYUP;
                u.key.keysym.sym = d.key.keysym.sym;
                SDL_PushEvent(&d);
                SDL_PushEvent(&u);
                started = true;
            }
        }
        Intents frame = input_->poll(save_->settings.mouseDial);
        if (frame.fullscreenPressed) {
            save_->settings.fullscreen = !save_->settings.fullscreen;
            SDL_SetWindowFullscreen(win_, save_->settings.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
        latch(frame);

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - pc) / SDL_GetPerformanceFrequency();
        pc = now;
        if (dt > 0.25f) dt = 0.25f;
        acc_ += dt;
        float alpha = 0;
        while (acc_ >= step) {
            tick(step);
            acc_ -= step;
        }
        alpha = acc_ / step;
        render(alpha);
        input_->endFrame();
    }

    save_->save();
    shutdownSDL();
    delete world_; delete demo_; delete fx_; delete audio_; delete input_;
    delete lr_; delete crt_; delete save_;
    return 0;
}

int Game::shot(const char* path, int level, int frames, bool safe) {
    if (initSDL(true)) return 1;
    save_ = new SaveData();
    save_->settings.safeMode = safe;
    win_ = SDL_CreateWindow("nv", 0, 0, w_, h_, 0);
    ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_SOFTWARE);
    if (!ren_) { fprintf(stderr, "renderer: %s\n", SDL_GetError()); shutdownSDL(); return 1; }
    VecFont::init();
    crt_ = new CrtPass();
    crt_->resize(ren_, w_, h_);
    lr_ = new LineRenderer();
    input_ = new Input();
    input_->init();
    audio_ = new AudioEngine(); // no audio subsystem in headless: stays silent
    fx_ = new Fx();
    world_ = new World();
    demo_ = new World();

    world_->startGame(level, 0);
    world_->phase = WorldPhase::ACTIVE;
    world_->activeTime = 2.5f;
    world_->time = 12.f;
    fx_->notifyLevel(level, world_->P.hash);
    state_ = PLAY;
    aiPlay_ = true;
    present_ = false;
    if (SDL_getenv("NV_HELP")) { // debug: capture the HOW TO PLAY screen
        state_ = ATTRACT;
        attractDemo_ = false;
        help_ = true;
        aiPlay_ = false;
    }
    if (SDL_getenv("NV_TITLE")) { // debug: capture the main screen
        state_ = ATTRACT;
        attractDemo_ = false;
        aiPlay_ = false;
    }

    const float step = 1.f / 60.f;
    for (int i = 0; i < frames; i++)
        tick(step);
    render(0.f); // single pass: first render clears the accumulators to black

    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w_, h_, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s) {
        SDL_RenderReadPixels(ren_, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
        printf("screenshot: %s (%dx%d, sector %d, safe=%d)\n", path, w_, h_, level + 1, safe ? 1 : 0);
    }
    shutdownSDL();
    delete world_; delete demo_; delete fx_; delete audio_; delete input_;
    delete lr_; delete crt_; delete save_;
    return 0;
}

void Game::startGame() {
    loop_ = 0;
    world_->startGame(0, 0);
    fx_->notifyLevel(0, world_->P.hash);
    audio_->play(Sfx::COIN);
    audio_->play(Sfx::LEVEL_START);
    state_ = PLAY;
    attractDemo_ = false;
}

void Game::demoStart(int idx) {
    demoIdx_ = ((idx % 4) + 4) % 4;
    demo_->startLevel(kDemoLevels[demoIdx_], 0, false);
    demo_->demo = true;
    fx_->notifyLevel(kDemoLevels[demoIdx_], demo_->P.hash);
}

void Game::latch(const Intents& f) {
    // held state is replaced each frame; edges accumulate until a tick consumes
    // them, so a press is never lost on a frame that runs no fixed tick
    intents_.rotAxis = f.rotAxis;
    intents_.fire = f.fire;
    fireLatch_ = fireLatch_ || f.fire; // a tap is never lost across 0-tick frames
    intents_.mouseDX = f.mouseDX;
    intents_.menuUpHeld = f.menuUpHeld;
    intents_.menuDownHeld = f.menuDownHeld;
    intents_.anyPressed |= f.anyPressed;
    intents_.startPressed |= f.startPressed;
    intents_.firePressed |= f.firePressed;
    intents_.pausePressed |= f.pausePressed;
    intents_.operatorPressed |= f.operatorPressed;
    intents_.fullscreenPressed |= f.fullscreenPressed;
    intents_.helpPressed |= f.helpPressed;
    intents_.superPressed |= f.superPressed;
    intents_.ringPressed |= f.ringPressed;
    intents_.menuUp |= f.menuUp;
    intents_.menuDown |= f.menuDown;
    if (f.digitPressed >= 0) intents_.digitPressed = f.digitPressed;
}

int Game::menuDir(const Intents& in, float dt) {
    if (in.menuUp || in.menuDown) {          // fresh press: move now
        menuRepeatT_ = 0.42f;                // initial delay before repeat
        return in.menuUp ? -1 : 1;
    }
    if (in.menuUpHeld || in.menuDownHeld) {  // held: auto-repeat
        if (menuRepeatT_ <= 0) menuRepeatT_ = 0.42f;
        menuRepeatT_ -= dt;
        if (menuRepeatT_ <= 0) {
            menuRepeatT_ = 0.11f;            // repeat rate
            return in.menuUpHeld ? -1 : 1;
        }
        return 0;
    }
    menuRepeatT_ = 0;
    return 0;
}

void Game::tick(float dt) {
    gameClock_ += dt;
    blinkT_ += dt;
    if (fx_) fx_->update(dt, save_->settings.safeMode);

    Intents in = intents_;
    in.fire = in.fire || fireLatch_; // deliver a latched tap to exactly one tick
    fireLatch_ = false;
    // consume edge intents: each press is processed by exactly one tick; held
    // state (fire, rotAxis) persists across the sub-ticks of this frame
    intents_.startPressed = intents_.firePressed = intents_.pausePressed = false;
    intents_.operatorPressed = intents_.fullscreenPressed = intents_.helpPressed = false;
    intents_.superPressed = intents_.ringPressed = intents_.anyPressed = false;
    intents_.menuUp = intents_.menuDown = false;
    intents_.digitPressed = -1;
    if (aiPlay_) in = Intents{}; // world AI is applied inside PLAY below

    // debug: NV_LOG=1 traces the input -> fire chain every half second
    static int logT = 0;
    if (SDL_getenv("NV_LOG") && ++logT % 30 == 0)
        fprintf(stderr, "[nv] state=%d phase=%d alive=%d latchFire=%d fire=%d bullets=%zu\n",
                state_, (int)world_->phase, world_->pl.alive ? 1 : 0,
                intents_.fire ? 1 : 0, in.fire ? 1 : 0, world_->bullets.size());

    // operator menu (F1) — attract & pause only
    if (in.operatorPressed && (state_ == ATTRACT || state_ == PAUSE) && !headless_) {
        operatorMenu_ = !operatorMenu_;
        audio_->play(Sfx::MENU);
        if (!operatorMenu_) save_->save();
    }
    if (operatorMenu_) {
        // options panel: up/down with auto-repeat, ENTER activates, ESC/0/F1 closes
        int d = menuDir(in, dt);
        if (d != 0) {
            opSel_ = (opSel_ + d + 5) % 5;
            audio_->play(Sfx::MENU);
        }
        int item = -1;
        if (in.digitPressed >= 1 && in.digitPressed <= 6)
            item = (in.digitPressed == 6) ? 4 : in.digitPressed - 1; // digits 1-6 direct
        else if (in.startPressed || in.firePressed)
            item = opSel_;                                           // ENTER activates
        if (item >= 0) {
            switch (item) {
            case 0: save_->settings.safeMode = !save_->settings.safeMode; break;
            case 1: save_->settings.crtFx = !save_->settings.crtFx; break;
            case 2: save_->settings.mouseDial = !save_->settings.mouseDial; break;
            case 3: save_->settings.volMaster = std::min(1.f, save_->settings.volMaster + 0.1f); break;
            case 4: save_->resetScores(); break;
            }
            audio_->setVolumes(save_->settings.volMaster, save_->settings.volMusic,
                               save_->settings.volSfx, save_->settings.volDrone);
            audio_->play(Sfx::MENU);
        }
        if (in.digitPressed == 0 || in.pausePressed) {
            operatorMenu_ = false;
            save_->save();
        }
        return;
    }

    // HOW TO PLAY overlay (H) — attract & pause
    if (!headless_ && !operatorMenu_ && in.helpPressed &&
        (state_ == ATTRACT || state_ == PAUSE)) {
        help_ = !help_;
        audio_->play(Sfx::MENU);
    }
    if (help_) {
        if (in.pausePressed || in.startPressed) help_ = false; // Esc / Enter closes
        return;
    }

    switch (state_) {
    case ATTRACT:
        attractT_ += dt;
        if (!attractDemo_) {
            // main menu: up/down with auto-repeat, ENTER/fire selects
            int d = menuDir(in, dt);
            if (d != 0) {
                menuSel_ = (menuSel_ + d + 5) % 5;
                audio_->play(Sfx::MENU);
            }
            if (in.startPressed || in.firePressed) {
                switch (menuSel_) {
                case 0: // START GAME
                    startGame();
                    break;
                case 1: // SAFE MODE toggle
                    save_->settings.safeMode = !save_->settings.safeMode;
                    save_->save();
                    audio_->play(Sfx::MENU);
                    break;
                case 2: // CRT FX toggle
                    save_->settings.crtFx = !save_->settings.crtFx;
                    save_->save();
                    audio_->play(Sfx::MENU);
                    break;
                case 3: // HOW TO PLAY
                    help_ = true;
                    audio_->play(Sfx::MENU);
                    break;
                case 4: // DIAGNOSTICS
                    operatorMenu_ = true;
                    audio_->play(Sfx::MENU);
                    break;
                }
            }
            if (attractT_ > 20.f) {
                attractDemo_ = true;
                attractT_ = 0;
                demoStart(demoIdx_);
            }
        } else {
            demo_->update(dt, demo_->demoIntents(), *audio_, *fx_);
            audio_->setMusic(demo_->P.level, demo_->threat * 0.35f);
            if (attractT_ > 40.f) {
                attractDemo_ = false;
                attractT_ = 0;
            }
            if (demo_->result != WorldResult::NONE) demoStart(demoIdx_ + 1);
            if (in.startPressed || in.fire) startGame(); // quick-start from the demo
        }
        break;

    case PLAY: {
        Intents play = aiPlay_ ? world_->demoIntents() : in;
        world_->update(dt, play, *audio_, *fx_);
        audio_->setMusic(world_->P.level, world_->threat);
        if (world_->result == WorldResult::LEVEL_CLEAR) {
            if (world_->P.level >= 99) {
                state_ = VICTORY;
                victoryT_ = 3.5f;
                audio_->play(Sfx::LEVEL_CLEAR);
            } else {
                world_->startLevel(world_->P.level + 1, loop_, true);
                fx_->notifyLevel(world_->P.level, world_->P.hash);
                audio_->play(Sfx::LEVEL_START);
            }
        } else if (world_->result == WorldResult::GAME_OVER) {
            state_ = GAMEOVER;
            gameoverT_ = 0;
            audio_->play(Sfx::GAMEOVER);
            audio_->setMusic(0, 0.05f);
        } else if (in.pausePressed) {
            state_ = PAUSE;
            audio_->play(Sfx::MENU);
        }
        break;
    }

    case PAUSE:
        fx_->update(dt, save_->settings.safeMode);
        if (in.pausePressed || in.startPressed) {
            state_ = PLAY;
            audio_->play(Sfx::MENU);
        }
        break;

    case GAMEOVER:
        gameoverT_ += dt;
        if (gameoverT_ > 3.2f) {
            if (save_->qualifies(world_->score)) {
                state_ = INITIALS;
                iniSlot_ = 0;
                iniLetter_ = 0;
                initials_[0] = initials_[1] = initials_[2] = 'A';
            } else {
                state_ = ATTRACT;
                attractT_ = 0;
                attractDemo_ = false;
            }
        }
        break;

    case INITIALS: {
        iniRepeatT_ -= dt;
        if (in.rotAxis != 0 && iniRepeatT_ <= 0) {
            iniLetter_ = ((iniLetter_ + (in.rotAxis > 0 ? 1 : -1)) % 26 + 26) % 26;
            initials_[iniSlot_] = (char)('A' + iniLetter_);
            iniRepeatT_ = 0.15f;
            audio_->play(Sfx::MENU, 1.f, 0.5f);
        }
        if (in.startPressed) {
            iniSlot_++;
            iniLetter_ = 0;
            audio_->play(Sfx::MENU);
            if (iniSlot_ >= 3) {
                save_->insert(world_->score, initials_);
                state_ = ATTRACT;
                attractT_ = 0;
                attractDemo_ = false;
            }
        }
        if (in.pausePressed) { // skip
            state_ = ATTRACT;
            attractT_ = 0;
            attractDemo_ = false;
        }
        break;
    }

    case VICTORY:
        victoryT_ -= dt;
        if (victoryT_ <= 0) {
            loop_++;
            world_->startLevel(0, loop_, true);
            fx_->notifyLevel(0, world_->P.hash);
            audio_->play(Sfx::LEVEL_START);
            state_ = PLAY;
        }
        break;
    }
}

// ---- rendering -------------------------------------------------------------

void Game::drawTitle(float alpha) {
    (void)alpha;
    Palette pal = makePalette(world_->P);
    float cx = w_ * 0.5f;
    float logoSize = h_ * 0.095f + std::sin(gameClock_ * 4.f) * h_ * 0.003f;
    VecFont::draw(*lr_, "NEURO-VECTOR", cx + 2, h_ * 0.20f, logoSize,
                  withA(pal.rim, 0.35f), VecFont::CENTER, 1);
    VecFont::draw(*lr_, "NEURO-VECTOR", cx, h_ * 0.20f, logoSize, pal.ui, VecFont::CENTER, 2);
    VecFont::draw(*lr_, "SENSE-WIPE SYSTEM v1.4", cx, h_ * 0.30f, h_ * 0.026f,
                  withA(pal.rim, 0.9f), VecFont::CENTER, 1);
    VecFont::draw(*lr_, "(C) 1981 SINNESTIER CORP.", cx, h_ * 0.345f, h_ * 0.019f,
                  withA(pal.ui, 0.4f), VecFont::CENTER, 0);

    // single main menu: game start, safety toggles, help, diagnostics
    char safe[40], crt[40];
    snprintf(safe, sizeof safe, "SAFE MODE: %s", save_->settings.safeMode ? "ON" : "OFF");
    snprintf(crt, sizeof crt, "CRT FX: %s", save_->settings.crtFx ? "ON" : "OFF");
    const char* kMenuItems[5] = {"START GAME", safe, crt, "HOW TO PLAY", "DIAGNOSTICS"};
    float mfs = h_ * 0.037f;
    float my = h_ * 0.43f;
    for (int i = 0; i < 5; i++) {
        float iy = my + i * mfs * 1.85f;
        bool sel = (i == menuSel_);
        Color c = sel ? pal.ui : withA(pal.ui, 0.5f);
        int glow = sel ? (std::fmod(blinkT_, 0.9f) < 0.6f ? 2 : 1) : 0;
        VecFont::draw(*lr_, kMenuItems[i], cx, iy, mfs, c, VecFont::CENTER, glow);
        if (sel) {
            float tw = VecFont::measure(kMenuItems[i], mfs);
            drawShipIcon(*lr_, {cx - tw * 0.5f - mfs * 1.7f, iy - mfs * 0.32f},
                         mfs * 0.55f, pal.player);
        }
    }

    if (std::fmod(blinkT_, 1.f) < 0.55f)
        VecFont::draw(*lr_, "INSERT COIN", cx, h_ * 0.80f, h_ * 0.035f,
                      withA(pal.ui, 0.95f), VecFont::CENTER, 1);

    // photosensitivity warning — always visible on the main screen
    Color warn = rgba(255, 110, 70, 200);
    VecFont::draw(*lr_, "WARNING: FLASHING LIGHTS - PHOTOSENSITIVE EPILEPSY RISK", cx, h_ * 0.88f,
                  h_ * 0.020f, warn, VecFont::CENTER, 0);
    VecFont::draw(*lr_, "IF SENSITIVE, TURN SAFE MODE ON", cx, h_ * 0.915f,
                  h_ * 0.020f, withA(warn, 0.65f), VecFont::CENTER, 0);

    char buf[64];
    VecFont::draw(*lr_, "HIGH SCORES", w_ * 0.95f, h_ * 0.52f, h_ * 0.026f,
                  withA(pal.rim, 0.9f), VecFont::RIGHT, 1);
    for (int i = 0; i < save_->count; i++) {
        snprintf(buf, sizeof buf, "%d. %s %07d", i + 1, save_->scores[i].initials, save_->scores[i].score);
        VecFont::draw(*lr_, buf, w_ * 0.95f, h_ * (0.555f + i * 0.033f), h_ * 0.022f,
                      withA(pal.ui, 0.75f), VecFont::RIGHT, 0);
    }
    VecFont::draw(*lr_, "F1: DIAGNOSTICS", w_ * 0.97f, h_ * 0.96f, h_ * 0.018f,
                  withA(pal.ui, 0.35f), VecFont::RIGHT, 0);
    VecFont::draw(*lr_, "UP/DOWN + ENTER", w_ * 0.03f, h_ * 0.96f, h_ * 0.018f,
                  withA(pal.ui, 0.35f), VecFont::LEFT, 0);
}

void Game::drawSceneWorld(float alpha) {
    World* w = (state_ == ATTRACT && attractDemo_) ? demo_ : world_;
    Palette pal = makePalette(w->P);
    float sh = fx_->shake();
    float t = w->time;
    float shX = std::sin(t * 57.3f) * sh * 10.f;
    float shY = std::cos(t * 47.7f) * sh * 8.f;

    switch (state_) {
    case ATTRACT:
        if (!attractDemo_) drawTitle(alpha);
        else drawWorld(*lr_, *w, pal, (float)w_, (float)h_, alpha, shX, shY);
        break;
    default:
        drawWorld(*lr_, *w, pal, (float)w_, (float)h_, alpha, shX, shY);
        break;
    }
}

void Game::drawSceneHud(float alpha) {
    (void)alpha;
    World* w = (state_ == ATTRACT && attractDemo_) ? demo_ : world_;
    Palette pal = makePalette(w->P);
    char buf[96];
    float cx = w_ * 0.5f;

    switch (state_) {
    case ATTRACT:
        if (attractDemo_) drawHud(*lr_, *w, (float)w_, (float)h_, blinkT_, true);
        break;
    case GAMEOVER:
        drawHud(*lr_, *w, (float)w_, (float)h_, blinkT_, false);
        VecFont::draw(*lr_, "GAME OVER", cx, h_ * 0.45f, h_ * 0.085f, withA(pal.ui, 0.95f), VecFont::CENTER, 2);
        break;
    case VICTORY:
        drawHud(*lr_, *w, (float)w_, (float)h_, blinkT_, false);
        VecFont::draw(*lr_, "SYSTEM PURGED", cx, h_ * 0.42f, h_ * 0.06f, withA(pal.ui, 0.95f), VecFont::CENTER, 2);
        snprintf(buf, sizeof buf, "LOOP %d BEGINS", loop_ + 1);
        VecFont::draw(*lr_, buf, cx, h_ * 0.52f, h_ * 0.035f, withA(pal.rim, 0.9f), VecFont::CENTER, 1);
        break;
    case INITIALS: {
        drawHud(*lr_, *w, (float)w_, (float)h_, blinkT_, false);
        VecFont::draw(*lr_, "NEW HIGH SCORE", cx, h_ * 0.30f, h_ * 0.05f, withA(pal.ui, 0.95f), VecFont::CENTER, 2);
        VecFont::draw(*lr_, "ENTER INITIALS", cx, h_ * 0.40f, h_ * 0.03f, withA(pal.rim, 0.8f), VecFont::CENTER, 1);
        float sp = h_ * 0.07f;
        for (int i = 0; i < 3; i++) {
            char l[2] = {initials_[i], 0};
            bool cur = (i == iniSlot_) && std::fmod(blinkT_, 0.8f) < 0.55f;
            VecFont::draw(*lr_, l, cx + (i - 1) * sp, h_ * 0.52f, h_ * 0.06f,
                          (i == iniSlot_) ? pal.ui : withA(pal.ui, 0.5f), VecFont::CENTER, cur ? 2 : 1);
        }
        snprintf(buf, sizeof buf, "%07d", world_->score);
        VecFont::draw(*lr_, buf, cx, h_ * 0.62f, h_ * 0.035f, withA(pal.rim, 0.9f), VecFont::CENTER, 1);
        break;
    }
    default:
        drawHud(*lr_, *w, (float)w_, (float)h_, blinkT_, false);
        break;
    }
}

void Game::drawOverlays() {
    lr_->begin(ren_);
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);

    // subliminal layer: 1-2 frame flashes of cryptic phrases / peripheral static
    if (fx_->subliminalActive() && !save_->settings.safeMode) {
        float a = 0.10f;
        if (!fx_->subliminalStatic()) {
            const char* p = kPhrases[fx_->subliminalPhrase() % 6];
            VecFont::draw(*lr_, p, w_ * 0.5f, h_ * 0.5f, h_ * 0.12f,
                          rgba(255, 255, 255, (int)(a * 255)), VecFont::CENTER, 0);
        } else {
            Rng r((uint64_t)(gameClock_ * 997.f));
            for (int i = 0; i < 40; i++) {
                float x = r.f01() * w_, y = r.f01() * h_;
                if (x > w_ * 0.18f && x < w_ * 0.82f && y > h_ * 0.18f && y < h_ * 0.82f)
                    continue; // edges only
                float len = r.range(4.f, 40.f);
                lr_->line(x, y, x + len, y + r.range(-4.f, 4.f),
                          rgba(255, 255, 255, (int)(a * 255)), 0);
            }
        }
    }

    if (state_ == PAUSE) {
        lr_->fillRect(0, 0, (float)w_, (float)h_, rgba(0, 0, 0, 140));
        Palette pal = makePalette(world_->P);
        VecFont::draw(*lr_, "PAUSE", w_ * 0.5f, h_ * 0.46f, h_ * 0.07f,
                      withA(pal.ui, 0.95f), VecFont::CENTER, 2);
        VecFont::draw(*lr_, "ESC: RESUME   F1: DIAGNOSTICS", w_ * 0.5f, h_ * 0.56f, h_ * 0.024f,
                      withA(pal.ui, 0.55f), VecFont::CENTER, 0);
    }

    if (operatorMenu_) {
        lr_->fillRect(0, 0, (float)w_, (float)h_, rgba(0, 0, 0, 210)); // dim the scene
        drawOperatorMenu(*lr_, (float)w_, (float)h_, gameClock_, save_->settings, save_->count, opSel_);
    }

    if (help_) {
        // modal: blank the scene so the title/HUD cannot ghost through
        lr_->fillRect(0, 0, (float)w_, (float)h_, rgba(0, 0, 0, 255));
        drawHelpScreen(*lr_, (float)w_, (float)h_, makePalette(world_->P));
    }
}

void Game::render(float alpha) {
    bool safe = save_->settings.safeMode;
    // beat envelope -> tunnel pulse
    float bpm = (state_ == ATTRACT && attractDemo_) ? demo_->P.bpm : world_->P.bpm;
    float phase = audio_->enabled()
                      ? audio_->beatPhase()
                      : std::fmod(gameClock_ * bpm / 60.f, 1.f);
    float env = std::pow(1.f - phase, 2.2f);
    world_->pulseEnv = env;
    demo_->pulseEnv = env;

    if (save_->settings.crtFx) {
        crt_->beginWorld(ren_, safe ? 0.78f : 0.86f);
        lr_->begin(ren_);
        drawSceneWorld(alpha);
        crt_->beginHud(ren_, safe ? 0.18f : 0.35f);
        lr_->begin(ren_);
        drawSceneHud(alpha);
        float aberrPx = fx_->aberration() * 6.f;
        crt_->present(ren_, aberrPx, true, safe ? 0.5f : 1.f);
        crt_->drawFlash(ren_, fx_->flash());
    } else {
        SDL_SetRenderTarget(ren_, nullptr);
        SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ren_, 0, 0, 0, 255);
        SDL_RenderClear(ren_);
        SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_ADD);
        lr_->begin(ren_);
        drawSceneWorld(alpha);
        drawSceneHud(alpha);
        crt_->drawFlash(ren_, fx_->flash());
    }

    drawOverlays();

    if (present_) {
        SDL_RenderPresent(ren_);
    } else if (!headless_) {
        // capture path only (headless capture saves before present)
    }
}

} // namespace nv
