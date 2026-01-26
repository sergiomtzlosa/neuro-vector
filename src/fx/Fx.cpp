#include "Fx.h"
#include <cmath>
#include <cstring>

namespace nv {

void Fx::notifyLevel(int level, uint64_t levelSeed) {
    (void)level;
    seed_ = levelSeed;
    Rng r(fnv1a(levelSeed, 555));
    subInterval_ = 45.f + 45.f * r.f01();
    subTimer_ = subInterval_;
    subPhrase_ = r.range(0, 5);
    subFrames_ = 0;
    subStatic_ = false;
}

void Fx::update(float dt, bool safeMode) {
    safeMode_ = safeMode;
    fxTime_ += dt;
    aberr_ = std::max(0.f, aberr_ - dt * 2.2f);
    shake_ = std::max(0.f, shake_ - dt * 1.8f);
    flash_ = std::max(0.f, flash_ - dt * 4.0f);

    // flash guard: count flashes in the last second
    flashCount1s_ = 0;
    for (float t : flashTimes_)
        if (t > 0 && fxTime_ - t < 1.f) flashCount1s_++;

    if (subFrames_ > 0) subFrames_--; // 1-2 frames at 60 Hz = 16-33 ms

    if (!safeMode) {
        subTimer_ -= dt;
        if (subTimer_ <= 0) {
            Rng r(fnv1a(seed_, (uint64_t)(subInterval_ * 1000.f)));
            subFrames_ = r.chance(0.5f) ? 2 : 1;
            subPhrase_ = r.range(0, 5);
            subStatic_ = r.chance(0.35f);
            subInterval_ = 45.f + 45.f * r.f01();
            subTimer_ = subInterval_;
        }
    } else {
        subFrames_ = 0; // Safe Mode disables the subliminal layer entirely
    }
}

bool Fx::allowFlash() {
    if (!safeMode_) return true;
    return flashCount1s_ < 3; // Safe Mode: cap strobes below 3 Hz
}

void Fx::registerFlash() {
    flashTimes_[flashCursor_ & 31] = fxTime_;
    flashCursor_++;
}

void Fx::damage() {
    aberr_ = 1.f;
    shake_ = 1.f;
    if (allowFlash()) { flash_ = std::max(flash_, 0.55f); registerFlash(); }
}

void Fx::hitPulse() {
    aberr_ = std::max(aberr_, 0.22f + multGlitch_ * 0.35f);
}

void Fx::superWave() {
    aberr_ = std::max(aberr_, 0.8f);
    shake_ = std::max(shake_, 0.45f);
    if (allowFlash()) { flash_ = std::max(flash_, 0.85f); registerFlash(); }
}

void Fx::warpPulse() {
    aberr_ = std::max(aberr_, 0.5f);
}

void Fx::setMult(float m) {
    multGlitch_ = clamp01((m - 24.f) / 40.f);
}

void Fx::forceSubliminal() {
    subFrames_ = 2;
    subStatic_ = false;
}

} // namespace nv
