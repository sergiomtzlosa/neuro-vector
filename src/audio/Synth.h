// Neuro-Vector — synth voice (oscillator + ADSR + Chamberlin SVF).
#pragma once
#include <cstdint>
#include <cmath>
#include "../core/Common.h"

namespace nv {

enum class Wave : int { SQUARE, SAW, TRI, SINE, NOISE };

struct VoiceParams {
    Wave  wave = Wave::SQUARE;
    float freq = 440;
    float glideTo = 0;    // 0 = no glide
    float glideRate = 0;  // per-second approach factor
    float attack = 0.005f, hold = 0.2f, release = 0.08f;
    float amp = 0.5f;
    float pan = 0.5f;     // 0 = L, 1 = R
    float cutoff = 4000, res = 0.2f;
    int   bus = 0;        // 0 = sfx, 1 = music, 2 = drone
};

struct Voice {
    bool  on = false;
    Wave  wave = Wave::SQUARE;
    double phase = 0;
    float freq = 440, targetFreq = 440, glideRate = 0;
    float t = 0, attack = 0.005f, hold = 0.2f, release = 0.08f;
    float amp = 0.5f, pan = 0.5f, cutoff = 4000, res = 0.2f;
    int   bus = 0;
    float lp = 0, band = 0;
    uint32_t nseed = 1;
    float sr = 48000;

    void trigger(const VoiceParams& p, uint32_t seed, float sampleRate) {
        on = true; wave = p.wave; phase = 0; freq = p.freq;
        targetFreq = p.glideTo > 0 ? p.glideTo : p.freq;
        glideRate = p.glideRate;
        t = 0; attack = p.attack; hold = p.hold; release = p.release;
        amp = p.amp; pan = p.pan; cutoff = p.cutoff; res = p.res;
        bus = p.bus; lp = 0; band = 0; nseed = seed ? seed : 1; sr = sampleRate;
    }

    bool step(float dt, const float busVols[3], float& outL, float& outR) {
        if (!on) return false;
        t += dt;
        float total = attack + hold + release;
        if (t >= total) { on = false; return false; }
        float env;
        if (t < attack) env = t / std::max(attack, 1e-5f);
        else if (t < attack + hold) env = 1;
        else env = std::max(0.f, 1.f - (t - attack - hold) / std::max(release, 1e-5f));
        env *= env; // snappier vector-monitor envelope

        if (glideRate > 0) freq += (targetFreq - freq) * std::min(1.f, glideRate * dt);
        phase += (double)dt * freq;
        float frac = (float)(phase - std::floor(phase));

        float v = 0;
        switch (wave) {
        case Wave::SQUARE: v = frac < 0.5f ? 1.f : -1.f; break;
        case Wave::SAW:    v = 2.f * frac - 1.f; break;
        case Wave::TRI:    v = frac < 0.5f ? 4.f * frac - 1.f : 3.f - 4.f * frac; break;
        case Wave::SINE:   v = std::sin(TAU * frac); break;
        case Wave::NOISE: {
            nseed = nseed * 1664525u + 1013904223u;
            v = (float)(int32_t)nseed / 2147483648.f;
            break;
        }
        }

        // Chamberlin state-variable lowpass
        float f = 2.f * std::sin(PI * std::min(0.24f, std::max(0.001f, cutoff / sr)));
        float high = v - lp - res * band;
        band += f * high;
        lp += f * band;
        float out = lp;

        float s = out * amp * env * busVols[bus];
        outL = s * (1.f - pan);
        outR = s * pan;
        return true;
    }
};

} // namespace nv
