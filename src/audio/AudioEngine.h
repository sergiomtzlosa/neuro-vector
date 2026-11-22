// Neuro-Vector — real-time audio engine (raw SDL callback synthesis).
#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "Synth.h"
#include "Sequencer.h"
#include "AudioTypes.h"

namespace nv {

class AudioEngine {
public:
    bool init();
    void shutdown();
    bool enabled() const { return dev_ != 0; }

    void setVolumes(float master, float music, float sfx, float drone);
    // Music follows the current sector (game thread; applied at next bar).
    void setMusic(int level, float intensity);
    void play(Sfx id, float pitch = 1.f, float vol = 1.f);

    // Sample-accurate beat phase in [0,1) within the beat (renderer sync).
    float beatPhase() const { return beatPhase_.load(std::memory_order_relaxed); }

private:
    void callback(Uint8* stream, int len);
    void spawnVoice(const VoiceParams& p);
    void handleSfx(const SfxEvent& e);
    void tickMusic(float dt);
    void regenMusic();

    SDL_AudioDeviceID dev_ = 0;
    SDL_AudioSpec spec_{};
    float sr_ = 48000;

    std::array<Voice, 128> voices_{};
    uint32_t cursor_ = 0;

    // SPSC sfx event ring (game thread -> audio thread)
    std::array<SfxEvent, 256> ring_{};
    std::atomic<uint32_t> ringHead_{0}, ringTail_{0};

    std::atomic<float> beatPhase_{0};
    std::atomic<int>   seqLevel_{0};
    std::atomic<int>   seqReq_{0};
    std::atomic<float> intensity_{0};
    std::atomic<float> vMaster_{0.8f}, vMusic_{0.65f}, vSfx_{0.9f}, vDrone_{0.5f};

    // audio-thread-only state
    Sequencer seq_{};
    int  seqLoaded_ = -1;
    int  seqReqSeen_ = 0;
    double stepAcc_ = 0;
    int  step_ = 0, bar_ = 0;
    double beatPos_ = 0;
    // drone
    double dPhaseA_ = 0, dPhaseB_ = 0, dPhaseF_ = 0, dLfo_ = 0;
    float busVols_[3] = {0.9f, 0.65f, 0.5f};
    uint32_t rngState_ = 0x1234abcd;
};

} // namespace nv
