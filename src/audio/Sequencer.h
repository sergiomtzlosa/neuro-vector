// Neuro-Vector — 16-step minor-key sequencer patterns (regenerated per level).
#pragma once
#include <cstdint>
#include "../core/Common.h"
#include "../core/Rng.h"

namespace nv {

// Natural-minor scale over two octaves (semitone offsets from the chord root).
constexpr int kMinorScale[14] = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22};
// Chord progression per bar: i - VI - III - VII (dark 1980s minor motion).
constexpr int kProgression[4] = {0, -4, 3, -2};

struct Sequencer {
    uint8_t arp[16] = {};    // index into kMinorScale, 255 = rest
    int8_t  bass[4] = {};    // chord root offsets (semitones)
    bool    kick[16] = {};
    bool    hat[16] = {};
    bool    snare[16] = {};
    bool    pad = false;     // tier-3 pad drone chord

    void regen(int level, uint64_t seed) {
        Rng r(fnv1a(seed, 777ull));
        for (int s = 0; s < 16; s++)
            arp[s] = r.chance(0.55f) ? (uint8_t)r.range(0, 13) : 255;
        for (int b = 0; b < 4; b++) bass[b] = (int8_t)kProgression[b];
        for (int s = 0; s < 16; s++) {
            kick[s] = (s % 4 == 0);                      // 4-on-the-floor
            hat[s]  = (s % 4 == 2) || (level >= 12 && s % 8 == 3);
            snare[s] = (level >= 33) && (s == 4 || s == 12);
        }
        pad = (level >= 40);
    }
};

} // namespace nv
