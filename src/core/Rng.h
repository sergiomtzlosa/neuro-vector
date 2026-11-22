// Neuro-Vector — deterministic RNG (Mersenne Twister, seeded per level).
//
// IMPORTANT: std::uniform_real_distribution and std::uniform_int_distribution
// are implementation-defined — libstdc++ (GCC/Linux), libc++ (Clang/macOS) and
// MSVC all produce different sequences from the same engine state.  To keep
// level hashes identical across every CI platform we use our own portable
// distribution helpers that depend only on std::mt19937::operator() (which IS
// specified by the standard).
#pragma once
#include <random>
#include <cstdint>
#include "Common.h"

namespace nv {

class Rng {
public:
    explicit Rng(uint64_t seed) : seed_(seed), mt(makeSeq(seed)) {}

    uint32_t u32() { return mt(); }

    // Portable [0, 1) float — uses the top 24 bits of a 32-bit draw so the
    // mantissa is fully populated, then divides by 2^24.
    float f01() {
        return (float)(mt() >> 8) * (1.0f / 16777216.0f); // 1 / 2^24
    }

    float range(float a, float b) { return a + (b - a) * f01(); }

    // Portable inclusive integer range [a, b] using rejection sampling to
    // eliminate modulo bias.  For the small ranges used by LevelGen (lanes,
    // enemy weights, etc.) this almost never rejects.
    int range(int a, int b) {
        uint32_t span = (uint32_t)(b - a) + 1u;
        uint32_t limit = ((uint32_t)(-1) / span) * span; // largest multiple
        uint32_t r;
        do { r = mt(); } while (r >= limit);
        return a + (int)(r % span);
    }

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
