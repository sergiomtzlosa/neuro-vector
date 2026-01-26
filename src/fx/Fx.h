// Neuro-Vector — sensory FX + photosensitivity FlashGuard (Safe Mode).
#pragma once
#include "../core/Common.h"
#include "../core/Rng.h"

namespace nv {

class Fx {
public:
    void notifyLevel(int level, uint64_t levelSeed);
    void update(float dt, bool safeMode);

    void damage();     // player hit: heavy aberration + shake + flash
    void hitPulse();   // enemy destroyed: small glitch pulse
    void superWave();  // smart bomb: big flash + aberration
    void warpPulse();  // ring jump: medium aberration

    float aberration() const { return aberr_; }
    float shake() const { return shake_; }
    float flash() const { return flash_; }
    // subliminal overlay state
    bool  subliminalActive() const { return subFrames_ > 0; }
    int   subliminalPhrase() const { return subPhrase_; }
    bool  subliminalStatic() const { return subStatic_; }

    void setMult(float m);

    // introspection for selftest
    float subliminalInterval() const { return subInterval_; }
    int   flashesLastSecond() const { return flashCount1s_; }
    void  forceSubliminal(); // selftest hook

private:
    void registerFlash();
    bool allowFlash();

    float aberr_ = 0, shake_ = 0, flash_ = 0, multGlitch_ = 0;
    float fxTime_ = 0;
    bool  safeMode_ = false;

    float subTimer_ = 60.f, subInterval_ = 60.f;
    int   subFrames_ = 0, subPhrase_ = 0;
    bool  subStatic_ = false;
    uint64_t seed_ = 1;

    float flashTimes_[32] = {};
    int   flashCursor_ = 0;
    int   flashCount1s_ = 0;
    Rng   rng_{99};
};

} // namespace nv
