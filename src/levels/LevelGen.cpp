#include "LevelGen.h"
#include <cstdio>
#include <cstring>

namespace nv {

static const char* polygonName(int lanes) {
    switch (lanes) {
    case 6:  return "HEXAGON";
    case 7:  return "HEPTAGON";
    case 8:  return "OCTAGON";
    case 9:  return "NONAGON";
    case 10: return "DECAGON";
    case 11: return "HENDECAGON";
    case 12: return "DODECAGON";
    case 13: return "TRIDECAGON";
    case 14: return "TETRADECAGON";
    case 15: return "PENTADECAGON";
    default: return "HEXADECAGON";
    }
}

static const char* warpName(int mode) {
    switch (mode) {
    case 1: return "GRID";        // superellipse
    case 2: return "LEMNISCATE";  // lobed
    case 3: return "INFINITY";    // infinity warp
    default: return "POLYGON";
    }
}

LevelParams LevelGen::generate(int level, int loop) {
    LevelParams p;
    p.level = level;
    p.loop = loop;

    uint64_t seed = fnv1a(BASE_SEED, (uint64_t)(level + 1) * 0x9E3779B97F4A7C15ull);
    seed = fnv1a(seed, (uint64_t)loop + 1);
    Rng r(seed);
    Rng rg = r.fork(1); // geometry
    Rng rw = r.fork(2); // waves + enemy mix
    Rng rp = r.fork(3); // palette + audio

    p.tier = level < 33 ? 0 : (level < 66 ? 1 : 2);

    // Lane count progression: 6 -> 16 (triangle..hexadecagon family, plan: closed polygons).
    if (p.tier == 0)      p.lanes = 6 + level / 11;                     // 6..8
    else if (p.tier == 1) p.lanes = std::min(12, 10 + (level - 33) / 12); // 10..12
    else                  p.lanes = std::min(16, 12 + (level - 66) / 12); // 12..15(16 on L100)

    p.spin = (rg.chance(0.5f) ? 1.f : -1.f) * std::min(0.6f, 0.12f + level * 0.010f);
    p.nested = (p.tier == 2);
    p.rollAmp  = p.nested ? 0.22f + 0.26f * rg.f01() : 0;
    p.rollFreq = 0.25f + 0.35f * rg.f01();
    p.pulse = 0.02f + 0.02f * rg.f01() + level * 0.0002f;

    // Warp geometry: tier 0 keeps pure polygons; tiers 1-2 warp the rim.
    if (p.tier == 0) {
        p.warpMode = 0;
    } else {
        p.warpMode = 1 + rw.range(0, 2);
        if (p.tier == 2 && rw.chance(0.35f)) p.warpMode = 0; // some hyper sectors stay pure
        p.warpK  = 1 + rw.range(0, 2);          // lobes
        p.warpP  = 0.5f + 1.5f * rw.f01();      // sharpness
        p.warpPh = TAU * rw.f01();
    }

    p.waves = 3 + rw.range(0, 1) + (level >= 20 ? 1 : 0); // 3..5
    p.enemiesPerWave = std::min(30, 6 + level / 2 + (p.waves - 3));
    p.spawnInterval = std::max(0.45f, 1.15f - level * 0.007f);
    p.waveGap = p.enemiesPerWave * p.spawnInterval * 0.55f;
    p.speedMul = 1.f + level * 0.02f + loop * 0.15f;

    p.hue = rp.f01();
    p.hueSpread = 0.08f + 0.10f * rp.f01();
    p.bpm = std::min(150.f, 112.f + level * 0.4f);

    // Snap every float that enters the hash to its hash-grid resolution.
    // This makes the hash a pure function of integers regardless of
    // platform-specific intermediate FP rounding (MSVC FMA, x87, etc.).
    auto snap = [](float v, float scale) -> float {
        return (float)lrintf(v * scale) / scale;
    };
    p.spin          = snap(p.spin,          8192);
    p.spawnInterval = snap(p.spawnInterval, 8192);
    p.speedMul      = snap(p.speedMul,      8192);
    p.warpK         = snap(p.warpK,          512);
    p.warpP         = snap(p.warpP,          512);
    p.warpPh        = snap(p.warpPh,        4096);
    p.hue           = snap(p.hue,           8192);
    p.bpm           = snap(p.bpm,             64);

    // Recompute waveGap from the snapped spawnInterval so the spawn
    // timeline is fully consistent with the hash grid.
    p.waveGap = p.enemiesPerWave * p.spawnInterval * 0.55f;
    p.superStart = 1;

    if (p.tier == 0) p.geoName = polygonName(p.lanes);
    else if (p.tier == 1) p.geoName = warpName(p.warpMode);
    else p.geoName = (p.warpMode ? "HYPER-" : "HYPER ") + std::string(p.warpMode ? warpName(p.warpMode) : polygonName(p.lanes));

    // Enemy mix unlocks across the 100 sectors.
    int w[(int)EnemyKind::COUNT] = {0};
    w[(int)EnemyKind::SEEKER]  = 10;
    if (level >= 2)  w[(int)EnemyKind::FLIPPER]  = 4 + level / 20;
    if (level >= 6)  w[(int)EnemyKind::SPLITTER] = 4 + level / 25;
    if (level >= 12) w[(int)EnemyKind::WEAVER]   = 3 + level / 30;
    if (level >= 18) w[(int)EnemyKind::PULSAR]   = 3 + level / 30;
    if (level >= 25) w[(int)EnemyKind::MIRROR]   = 2 + level / 40;
    int total = 0;
    for (int i = 0; i < (int)EnemyKind::COUNT; i++) total += w[i];

    // Full level script, deterministic.
    // Pre-compute the per-wave decay multiplier iteratively instead of calling
    // std::pow, whose implementation is not identical across MSVC/GCC/Clang.
    // Use double for the time accumulator so that repeated float additions
    // don't drift differently across platforms.
    double t = 0;
    float decay = 1.0f;                       // 0.93^0
    for (int wv = 0; wv < p.waves; wv++) {
        double interval = (double)p.spawnInterval * (double)decay;
        decay *= 0.93f;                       // 0.93^(wv+1) for next iteration
        t = (double)wv * (double)p.waveGap;
        for (int i = 0; i < p.enemiesPerWave; i++) {
            int pick = rw.range(0, std::max(1, total) - 1);
            EnemyKind kind = EnemyKind::SEEKER;
            for (int k = 0; k < (int)EnemyKind::COUNT; k++) {
                if (pick < w[k]) { kind = (EnemyKind)k; break; }
                pick -= w[k];
            }
            Spawn s;
            s.t = (float)t;
            s.kind = kind;
            s.lane = rw.range(0, p.lanes - 1);
            s.track = (p.nested && rw.chance(0.45f)) ? 1 : 0;
            p.spawns.push_back(s);
            t += interval;
        }
    }

    // Layout hash (determinism pin).
    // Use lrintf (round-to-nearest) instead of (int) truncation so that
    // sub-ULP floating-point differences across compilers don't flip the
    // integer by ±1 and break the hash.
    uint64_t h = fnv1a(BASE_SEED, (uint64_t)level + 1);
    h = fnv1a(h, (uint64_t)p.lanes);
    h = fnv1a(h, (uint64_t)lrintf(p.spin * 8192));
    h = fnv1a(h, (uint64_t)p.waves);
    h = fnv1a(h, (uint64_t)p.enemiesPerWave);
    h = fnv1a(h, (uint64_t)lrintf(p.spawnInterval * 8192));
    h = fnv1a(h, (uint64_t)lrintf(p.speedMul * 8192));
    h = fnv1a(h, (uint64_t)p.warpMode);
    h = fnv1a(h, (uint64_t)lrintf(p.warpK * 512));
    h = fnv1a(h, (uint64_t)lrintf(p.warpP * 512));
    h = fnv1a(h, (uint64_t)lrintf(p.warpPh * 4096));
    h = fnv1a(h, (uint64_t)lrintf(p.hue * 8192));
    h = fnv1a(h, (uint64_t)lrintf(p.bpm * 64));
    h = fnv1a(h, (uint64_t)p.spawns.size());
    for (const Spawn& s : p.spawns) {
        h = fnv1a(h, (uint64_t)lrintf(s.t * 1000));
        h = fnv1a(h, (uint64_t)((int)s.kind * 64 + s.lane * 4 + s.track));
    }
    p.hash = h;
    return p;
}

std::string LevelGen::hashText(const LevelParams& p) {
    char buf[128];
    snprintf(buf, sizeof buf, "%03d L%d T%d %016llx", p.level + 1, p.loop, p.tier,
             (unsigned long long)p.hash);
    return buf;
}

} // namespace nv
