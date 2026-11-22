// Neuro-Vector — shared math, types and constants.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace nv {

constexpr float PI  = 3.14159265358979f;
constexpr float TAU = 6.28318530717958f;
// Fixed base seed for the 100-level deterministic universe ("NV26").
constexpr uint64_t BASE_SEED = 0x4E563236ull;

struct Vec2 { float x = 0, y = 0; };
inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float wrap01(float v) { v -= std::floor(v); return v; }
inline float wrapF(float v, float m) { v = std::fmod(v, m); if (v < 0) v += m; return v; }
inline float wrapAngle(float a) { while (a > PI) a -= TAU; while (a < -PI) a += TAU; return a; }

struct Color { uint8_t r = 255, g = 255, b = 255, a = 255; };
inline Color rgba(int r, int g, int b, int a = 255) {
    return Color{(uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a};
}
inline Color withA(Color c, float a) { c.a = (uint8_t)(a * 255.f + 0.5f); return c; }

// h in [0,1), s/l/a in [0,1]
inline Color hsl(float h, float s, float l, float a = 1.f) {
    h = wrap01(h);
    float c = (1 - std::fabs(2 * l - 1)) * s;
    float x = c * (1 - std::fabs(std::fmod(h * 6, 2) - 1));
    float m = l - c / 2;
    float r = 0, g = 0, b = 0;
    if (h < 1.f / 6)      { r = c; g = x; }
    else if (h < 2.f / 6) { r = x; g = c; }
    else if (h < 3.f / 6) { g = c; b = x; }
    else if (h < 4.f / 6) { g = x; b = c; }
    else if (h < 5.f / 6) { r = x; b = c; }
    else                  { r = c; b = x; }
    return rgba(int((r + m) * 255), int((g + m) * 255), int((b + m) * 255), int(a * 255));
}

inline uint64_t fnv1a(uint64_t h, uint64_t v) { h ^= v; h *= 0x100000001b3ull; return h; }

} // namespace nv
