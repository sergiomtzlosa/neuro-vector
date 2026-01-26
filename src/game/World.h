// Neuro-Vector — gameplay world: player, enemies, waves, scoring.
#pragma once
#include <vector>
#include "../core/Common.h"
#include "../core/Rng.h"
#include "../core/Input.h"
#include "../game/Types.h"
#include "../levels/LevelGen.h"

namespace nv {

class AudioEngine;
class Fx;

enum class WorldPhase { INTRO, ACTIVE, OUTRO };
enum class WorldResult { NONE, LEVEL_CLEAR, GAME_OVER };

struct Enemy {
    EnemyKind kind = EnemyKind::SEEKER;
    int   lane = 0, track = 0;
    float depth = 1.f, prevDepth = 1.f;
    float speed = 0.15f;
    float flipT = 0, zapT = 0, zapHold = 0, warpT = 0, rimT = 0, age = 0, wob = 0;
    bool  alive = false;
    bool  zapping = false; // pulsar
    bool  warping = false; // weaver
};

struct Bullet {
    int   lane = 0, track = 0;
    float depth = 0, prevDepth = 0;
    bool  alive = false;
};

// Deferred spawn: processed after all enemy/bullet iteration (avoids
// push_back-during-range-for iterator invalidation).
struct DeferredSpawn {
    EnemyKind kind;
    int   lane, track;
    float depth, speedMul;
};

struct Player {
    float laneF = 0, prevLaneF = 0;
    int   lane = 0, track = 0;
    bool  alive = true;
    float invulnT = 0, respawnT = 0, fireCd = 0;
};

class World {
public:
    LevelParams P;
    WorldPhase  phase = WorldPhase::INTRO;
    WorldResult result = WorldResult::NONE;

    Player pl;
    std::vector<Enemy>  enemies;
    std::vector<Bullet> bullets;
    std::vector<DeferredSpawn> deferred_; // shards queued by killEnemy
    bool   pendingClear_ = false;         // arcade wipe, applied after iteration
    size_t spawnCursor = 0;
    int    waveIndex = -1;
    float  waveFlashT = 0;
    int    lives = 3;
    int    score = 0, chain = 0, mult = 1, supers = 1;
    int    nextExtra = 20000, nextSuper = 30000;
    float  time = 0, prevTime = 0;       // total (spin/roll)
    float  activeTime = 0;               // spawn clock (ACTIVE only)
    float  introT = 0, outroT = 0, outroBonus = 0;
    float  superT = 0;                   // smart-bomb ring visual
    float  fireFlashT = 0;               // muzzle flash after the ship fires
    float  noRingT = 0;                  // ring-jump denied flash (no second ring)
    float  pulseEnv = 0;                 // beat envelope, set by Game
    float  threat = 0;                   // 0..1 -> drone intensity
    bool   demo = false;

    void startGame(int startLevel = 0, int loop = 0);
    void startLevel(int level, int loop, bool keepPlayer);
    void update(float dt, const Intents& in, AudioEngine& audio, Fx& fx);

    // attract-mode AI (deterministic via level rng)
    Intents demoIntents();

    float camSpinAt(float t) const { return P.spin * t; }
    float camRollAt(float t) const { return P.nested ? std::sin(t * P.rollFreq) * P.rollAmp : 0; }
    bool  mirrorActive() const;

private:
    void spawn(EnemyKind kind, int lane, int track, float depth = 1.02f, float speedMul = 1.f);
    void damagePlayer(AudioEngine& audio, Fx& fx);
    void killEnemy(Enemy& e, AudioEngine& audio, Fx& fx, bool scored);
    void addScore(int pts, AudioEngine& audio, Fx& fx);
    void addBonus(int pts, AudioEngine& audio);
    void updateEnemy(Enemy& e, float dt, AudioEngine& audio, Fx& fx);

    Rng rng_{1};
};

} // namespace nv
