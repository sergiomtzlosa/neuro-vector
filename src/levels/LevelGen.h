// Neuro-Vector — deterministic procedural level generator (levels 1..100).
//
// seed = hash(BASE_SEED, level, loop) -> std::mt19937 sub-streams:
//   fork(1) geometry, fork(2) waves/enemies, fork(3) palette.
// Every level is fully reproducible; layout hashes are pinned in tests/golden/.
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "../core/Common.h"
#include "../core/Rng.h"
#include "../game/Types.h"

namespace nv {

struct Spawn {
    float      t;     // seconds since level start
    EnemyKind  kind;
    int        lane;
    int        track; // 0 outer, 1 inner (tier 3 only)
};

struct LevelParams {
    int   level = 0;        // 0-based (displayed +1)
    int   loop = 0;
    int   tier = 0;
    int   lanes = 8;
    float spin = 0.2f;      // tunnel spin rad/s (signed)
    float rollAmp = 0;      // tier-3 camera roll
    float rollFreq = 0.4f;
    float pulse = 0.03f;    // beat pulse depth
    int   waves = 3;
    int   enemiesPerWave = 8;
    float spawnInterval = 1.0f;
    float waveGap = 6.0f;   // seconds between wave starts
    float speedMul = 1.0f;
    bool  nested = false;   // tier 3: inner/outer tracks
    int   warpMode = 0;     // 0 regular polygon, 1 superellipse, 2 lobe, 3 infinity
    float warpK = 1, warpP = 1, warpPh = 0;
    float hue = 0, hueSpread = 0.1f;
    float bpm = 120;
    int   superStart = 1;
    std::string geoName;
    uint64_t hash = 0;

    std::vector<Spawn> spawns; // full level script, sorted by t
};

class LevelGen {
public:
    static LevelParams generate(int level, int loop = 0);
    // Stable textual fingerprint used by tests/golden/level_hashes.txt.
    static std::string hashText(const LevelParams& p);
};

} // namespace nv
