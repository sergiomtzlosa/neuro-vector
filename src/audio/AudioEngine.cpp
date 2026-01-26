#include "AudioEngine.h"
#include <cmath>
#include <cstring>

namespace nv {

static VoiceParams vp(Wave w, float freq, float hold, float amp, float cutoff,
                      float glideTo = 0, float glideRate = 0, float attack = 0.004f,
                      float release = 0.06f, float pan = 0.5f, float res = 0.2f, int bus = 0) {
    VoiceParams p;
    p.wave = w; p.freq = freq; p.hold = hold; p.amp = amp; p.cutoff = cutoff;
    p.glideTo = glideTo; p.glideRate = glideRate; p.attack = attack;
    p.release = release; p.pan = pan; p.res = res; p.bus = bus;
    return p;
}

bool AudioEngine::init() {
    SDL_AudioSpec want{};
    want.freq = 48000;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = [](void* user, Uint8* stream, int len) {
        static_cast<AudioEngine*>(user)->callback(stream, len);
    };
    want.userdata = this;
    dev_ = SDL_OpenAudioDevice(nullptr, 0, &want, &spec_, 0);
    if (!dev_) return false;
    sr_ = (float)spec_.freq;
    SDL_PauseAudioDevice(dev_, 0);
    return true;
}

void AudioEngine::shutdown() {
    if (dev_) { SDL_CloseAudioDevice(dev_); dev_ = 0; }
}

void AudioEngine::setVolumes(float m, float mu, float s, float d) {
    vMaster_.store(m, std::memory_order_relaxed);
    vMusic_.store(mu, std::memory_order_relaxed);
    vSfx_.store(s, std::memory_order_relaxed);
    vDrone_.store(d, std::memory_order_relaxed);
}

void AudioEngine::setMusic(int level, float intensity) {
    seqLevel_.store(level, std::memory_order_relaxed);
    intensity_.store(clamp01(intensity), std::memory_order_relaxed);
    seqReq_.fetch_add(1, std::memory_order_relaxed);
}

void AudioEngine::play(Sfx id, float pitch, float vol) {
    uint32_t head = ringHead_.load(std::memory_order_relaxed);
    uint32_t tail = ringTail_.load(std::memory_order_acquire);
    uint32_t next = (head + 1) & 255;
    if (next == tail) return; // ring full: drop
    ring_[head] = SfxEvent{id, pitch, vol};
    ringHead_.store(next, std::memory_order_release);
}

void AudioEngine::spawnVoice(const VoiceParams& p) {
    Voice& v = voices_[cursor_ & 127]; // round-robin steal
    cursor_++;
    v.trigger(p, (rngState_ += 0x9E3779B9u) | 1, sr_);
}

void AudioEngine::handleSfx(const SfxEvent& e) {
    float p = e.pitch, v = e.vol;
    switch (e.id) {
    case Sfx::SHOOT:
        spawnVoice(vp(Wave::SQUARE, 900 * p, .05f, .30f * v, 4200, 300 * p, 30)); break;
    case Sfx::HIT:
        spawnVoice(vp(Wave::SQUARE, 200 * p, .04f, .28f * v, 1500)); break;
    case Sfx::SHARD:
        spawnVoice(vp(Wave::TRI, 620 * p, .06f, .25f * v, 3000, 980 * p, 18)); break;
    case Sfx::DEATH:
        spawnVoice(vp(Wave::NOISE, 300, .14f, .45f * v, 1400));
        spawnVoice(vp(Wave::SAW, 320 * p, .2f, .35f * v, 900, 70 * p, 10)); break;
    case Sfx::FLIP:
        spawnVoice(vp(Wave::SQUARE, 420 * p, .07f, .22f * v, 2500, 840 * p, 14)); break;
    case Sfx::ZAP:
        spawnVoice(vp(Wave::SAW, 110, .24f, .40f * v, 700));
        spawnVoice(vp(Wave::NOISE, 400, .18f, .20f * v, 900)); break;
    case Sfx::MIRROR:
        spawnVoice(vp(Wave::SINE, 220, .4f, .22f * v, 2000));
        spawnVoice(vp(Wave::SINE, 224, .4f, .22f * v, 2000)); break;
    case Sfx::WARP:
        spawnVoice(vp(Wave::SINE, 300, .16f, .35f * v, 3000, 900, 12)); break;
    case Sfx::PLAYER_HIT:
        spawnVoice(vp(Wave::NOISE, 200, .45f, .75f * v, 1000));
        spawnVoice(vp(Wave::SINE, 130, .5f, .70f * v, 400, 38, 6)); break;
    case Sfx::SUPER:
        spawnVoice(vp(Wave::SAW, 60, .38f, .60f * v, 3200, 1200, 10));
        spawnVoice(vp(Wave::NOISE, 300, .3f, .40f * v, 2000)); break;
    case Sfx::LEVEL_START:
        spawnVoice(vp(Wave::TRI, 220, .3f, .35f * v, 3000, 880, 6)); break;
    case Sfx::LEVEL_CLEAR:
        spawnVoice(vp(Wave::SINE, 660, .18f, .30f * v, 4000));
        spawnVoice(vp(Wave::SINE, 880, .25f, .30f * v, 4000));
        spawnVoice(vp(Wave::TRI, 990, .3f, .25f * v, 4000)); break;
    case Sfx::EXTRA:
        spawnVoice(vp(Wave::SQUARE, 880, .06f, .28f * v, 4000, 1320, 20));
        spawnVoice(vp(Wave::SQUARE, 1100, .12f, .25f * v, 4000, 1650, 20)); break;
    case Sfx::MENU:
        spawnVoice(vp(Wave::SQUARE, 700, .03f, .18f * v, 3500)); break;
    case Sfx::COIN:
        spawnVoice(vp(Wave::SQUARE, 990, .06f, .30f * v, 5000));
        spawnVoice(vp(Wave::SQUARE, 1320, .18f, .30f * v, 5000)); break;
    case Sfx::GAMEOVER:
        spawnVoice(vp(Wave::SAW, 220, .85f, .45f * v, 1200, 55, 3));
        spawnVoice(vp(Wave::NOISE, 100, .5f, .20f * v, 600)); break;
    default: break;
    }
}

void AudioEngine::regenMusic() {
    int level = seqLevel_.load(std::memory_order_relaxed);
    seq_.regen(level, fnv1a(BASE_SEED, (uint64_t)(level + 1)));
    seqLoaded_ = level;
}

void AudioEngine::tickMusic(float dt) {
    int req = seqReq_.load(std::memory_order_relaxed);
    if (req != seqReqSeen_) { seqReqSeen_ = req; regenMusic(); }

    int level = seqLevel_.load(std::memory_order_relaxed);
    float intensity = intensity_.load(std::memory_order_relaxed);
    float bpm = std::min(150.f, 112.f + level * 0.4f);
    float stepDur = 60.f / bpm / 4.f;

    beatPos_ += (double)dt * bpm / 60.0;
    if (beatPos_ >= 4.0) { beatPos_ -= 4.0; bar_++; }
    beatPhase_.store((float)std::fmod(beatPos_, 1.0), std::memory_order_relaxed);

    stepAcc_ += dt;
    if (stepAcc_ < stepDur) return;
    stepAcc_ -= stepDur;
    int s = step_; step_ = (step_ + 1) & 15;

    float root = 110.f * std::pow(2.f, seq_.bass[bar_ % 4] / 12.f);

    if (seq_.kick[s])
        spawnVoice(vp(Wave::SINE, 150, .12f, .85f, 600, 45, 60, .002f, .08f, .5f, .2f, 1));
    if (seq_.hat[s])
        spawnVoice(vp(Wave::NOISE, 500, .02f, .20f + .10f * intensity, 7500, 0, 0, .001f, .05f,
                      s % 2 ? .42f : .58f, .05f, 1));
    else if (intensity > 0.6f && (s % 4) == 1)
        spawnVoice(vp(Wave::NOISE, 500, .02f, .10f, 7500, 0, 0, .001f, .04f, .5f, .05f, 1));
    if (seq_.snare[s])
        spawnVoice(vp(Wave::NOISE, 500, .09f, .32f, 3200, 0, 0, .002f, .1f, .5f, .15f, 1));
    if (s == 0 || s == 3 || s == 6 || s == 10)
        spawnVoice(vp(Wave::SAW, root * 0.5f, .2f, .45f, 350.f + 250.f * intensity, 0, 0,
                      .004f, .06f, .5f, .35f, 1));
    if (seq_.arp[s] != 255) {
        int n = seq_.arp[s];
        int oct = (intensity > 0.65f && ((rngState_ = rngState_ * 1664525u + 1013904223u) & 3) == 0) ? 24 : 12;
        float f = root * std::pow(2.f, kMinorScale[n] / 12.f) * std::pow(2.f, oct / 12.f);
        float pan = 0.3f + 0.4f * ((n % 4) / 3.f);
        spawnVoice(vp(Wave::SQUARE, f, .09f, .20f + .06f * intensity, 2600, 0, 0, .003f, .08f,
                      pan, .15f, 1));
    }
    if (seq_.pad && s == 0) {
        spawnVoice(vp(Wave::SAW, root * 0.997f, 2.2f, .10f, 750, 0, 0, .4f, .5f, .35f, .3f, 1));
        spawnVoice(vp(Wave::SAW, root * 1.004f, 2.2f, .10f, 750, 0, 0, .4f, .5f, .65f, .3f, 1));
    }
}

void AudioEngine::callback(Uint8* stream, int len) {
    float* out = (float*)stream;
    int frames = len / (int)(2 * sizeof(float));
    std::memset(out, 0, len);
    float dt = 1.f / sr_;

    // drain sfx ring
    uint32_t tail = ringTail_.load(std::memory_order_relaxed);
    uint32_t head = ringHead_.load(std::memory_order_acquire);
    while (tail != head) {
        handleSfx(ring_[tail]);
        tail = (tail + 1) & 255;
        ringTail_.store(tail, std::memory_order_relaxed);
    }

    busVols_[0] = vSfx_.load(std::memory_order_relaxed);
    busVols_[1] = vMusic_.load(std::memory_order_relaxed);
    busVols_[2] = vDrone_.load(std::memory_order_relaxed);
    float master = vMaster_.load(std::memory_order_relaxed);

    tickMusic(dt);

    // sub-bass drone: 40-60 Hz binaural pair + faint fifth, threat-mapped
    float intensity = intensity_.load(std::memory_order_relaxed);
    float df = 40.f + 25.f * intensity;
    dLfo_ += dt * 0.13f;
    float damp = (0.16f + 0.20f * intensity) * (0.75f + 0.25f * std::sin(dLfo_)) * busVols_[2];
    dPhaseA_ += (double)dt * df;
    dPhaseB_ += (double)dt * df * 1.007;
    dPhaseF_ += (double)dt * df * 1.5;
    float dA = std::sin(TAU * (float)std::fmod(dPhaseA_, 1.0));
    float dB = std::sin(TAU * (float)std::fmod(dPhaseB_, 1.0));
    float dF = std::sin(TAU * (float)std::fmod(dPhaseF_, 1.0)) * 0.3f;
    float drone = (dA + dB + dF) * damp;

    for (Voice& v : voices_) {
        float l, r;
        if (!v.step(dt, busVols_, l, r)) continue;
        out[0] += l; out[1] += r;
    }
    out[0] += drone; out[1] += drone;

    for (int i = 0; i < frames * 2; i++) {
        float v = out[i] * master;
        out[i] = std::tanh(v) * 0.92f; // soft limiter
    }
}

} // namespace nv
