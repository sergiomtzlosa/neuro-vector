// Neuro-Vector — deterministic RNG (Mersenne Twister, seeded per level).
#pragma once
#include <random>
#include <cstdint>
#include "Common.h"

namespace nv {

class Rng {
public:
    explicit Rng(uint64_t seed) : seed_(seed), mt(makeSeq(seed)) {}

    uint32_t u32() { return mt(); }
    float f01() { return std::uniform_real_distribution<float>(0.f, 1.f)(mt); }
    float range(float a, float b) { return a + (b - a) * f01(); }
    int range(int a, int b) { return std::uniform_int_distribution<int>(a, b)(mt); }
    bool chance(float p) { return f01() < p; }
    // Deterministic sub-stream (e.g. geometry vs waves vs palette).
    Rng fork(uint64_t salt) const { return Rng(fnv1a(seed_, salt * 0x9E3779B97F4A7C15ull)); }
    uint64_t seed() const { return seed_; }

private:
    static std::mt19937 makeSeq(uint64_t s) {
        std::seed_seq seq{(uint32_t)s, (uint32_t)(s >> 32), (uint32_t)(s * 0x9E3779B9ull >> 32)};
        return std::mt19937(seq);
    }
    uint64_t seed_;
    std::mt19937 mt;
};

} // namespace nv
