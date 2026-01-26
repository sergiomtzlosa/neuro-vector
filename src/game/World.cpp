#include "World.h"
#include "../audio/AudioEngine.h"
#include "../fx/Fx.h"
#include <cmath>

namespace nv {

void World::startGame(int firstLevel, int loop) {
    lives = 3; score = 0; chain = 0; mult = 1; supers = 1;
    nextExtra = 20000; nextSuper = 30000;
    startLevel(firstLevel, loop, false);
}

void World::startLevel(int level, int loop, bool keepPlayer) {
    P = LevelGen::generate(level, loop);
    phase = WorldPhase::INTRO;
    result = WorldResult::NONE;
    time = 0; prevTime = 0; activeTime = 0;
    introT = 2.2f; outroT = 0; outroBonus = 0;
    spawnCursor = 0;
    waveIndex = -1; waveFlashT = 0;
    superT = 0;
    fireFlashT = 0;
    noRingT = 0;
    pendingClear_ = false;
    deferred_.clear();
    enemies.clear(); bullets.clear();
    rng_ = Rng(fnv1a(P.hash, 42));
    if (!keepPlayer) {
        pl.laneF = 0; pl.track = 0; pl.alive = true;
        pl.invulnT = 0; pl.respawnT = 0; pl.fireCd = 0;
    } else {
        pl.alive = true; pl.invulnT = 1.0f; pl.respawnT = 0;
    }
}

bool World::mirrorActive() const {
    for (const Enemy& e : enemies)
        if (e.alive && e.kind == EnemyKind::MIRROR && e.depth < 0.85f) return true;
    return false;
}

void World::spawn(EnemyKind kind, int lane, int track, float depth, float speedMul) {
    Enemy e;
    e.kind = kind;
    e.lane = wrapF((float)lane, (float)P.lanes);
    e.track = track;
    e.depth = e.prevDepth = depth;
    e.speed = kindStats[(int)kind].speed * P.speedMul * speedMul;
    e.alive = true;
    enemies.push_back(e);
}

Intents World::demoIntents() {
    Intents in;
    // find the deepest threat (prefer player's track)
    const Enemy* best = nullptr;
    for (const Enemy& e : enemies) {
        if (!e.alive) continue;
        if (!best) { best = &e; continue; }
        bool better = e.track == pl.track && best->track != pl.track;
        if (e.depth < best->depth && (e.track == pl.track || best->track != pl.track || e.depth < best->depth - 0.3f))
            better = true;
        if (better) best = &e;
    }
    if (best) {
        float target = best->lane + (best->kind == EnemyKind::WEAVER ? std::sin(best->wob) * 0.8f : 0);
        float diff = wrapF(target - pl.laneF + P.lanes * 0.5f, (float)P.lanes) - P.lanes * 0.5f;
        in.rotAxis = std::max(-1.f, std::min(1.f, diff * 4.f));
        in.fire = std::fabs(diff) < 0.4f && rng_.chance(0.85f);
        if (P.nested && best->track != pl.track && best->depth < 0.6f && rng_.chance(0.03f))
            in.ringPressed = true;
    }
    if (supers > 0 && (int)enemies.size() > 7 && rng_.chance(0.015f)) in.superPressed = true;
    return in;
}

void World::addScore(int pts, AudioEngine& audio, Fx& fx) {
    score += pts;
    chain++;
    mult = std::min(64, 1 + chain / 2);
    fx.setMult((float)mult);
    if (score >= nextExtra) {
        lives = std::min(6, lives + 1);
        nextExtra += (nextExtra == 20000) ? 30000 : 70000;
        audio.play(Sfx::EXTRA);
    }
    if (score >= nextSuper) {
        supers = std::min(3, supers + 1);
        nextSuper += 50000;
        audio.play(Sfx::EXTRA);
    }
}

void World::addBonus(int pts, AudioEngine& audio) {
    score += pts;
    audio.play(Sfx::LEVEL_CLEAR);
}

void World::killEnemy(Enemy& e, AudioEngine& audio, Fx& fx, bool scored) {
    e.alive = false;
    fx.hitPulse();
    static const float pitch[(int)EnemyKind::COUNT] = {1.0f, 1.2f, 0.8f, 1.5f, 0.7f, 0.6f, 1.3f};
    audio.play(Sfx::DEATH, pitch[(int)e.kind], 1.f);
    if (e.kind == EnemyKind::SPLITTER) {
        // deferred: spawning here would push_back into `enemies` while callers
        // are range-iterating it (iterator invalidation / heap corruption)
        deferred_.push_back({EnemyKind::SPLITTER_SHARD, e.lane - 1, e.track, e.depth + 0.12f, 1.2f});
        deferred_.push_back({EnemyKind::SPLITTER_SHARD, e.lane + 1, e.track, e.depth + 0.12f, 1.2f});
        audio.play(Sfx::SHARD);
    }
    if (scored) {
        float prox = 1.f + 2.f * (1.f - clamp01(e.depth)); // near-rim kills worth more
        addScore((int)(kindStats[(int)e.kind].score * mult * prox), audio, fx);
    }
}

void World::damagePlayer(AudioEngine& audio, Fx& fx) {
    if (!pl.alive || pl.invulnT > 0) return;
    fx.damage();
    audio.play(Sfx::PLAYER_HIT);
    chain = 0; mult = 1; fx.setMult(1);
    lives--;
    pl.alive = false;
    pl.respawnT = 2.2f;
    // deferred: clearing `enemies` here would invalidate the range-for in
    // update() that is currently calling updateEnemy() (heap corruption)
    pendingClear_ = true;
    if (lives < 0) result = WorldResult::GAME_OVER;
}

void World::updateEnemy(Enemy& e, float dt, AudioEngine& audio, Fx& fx) {
    e.prevDepth = e.depth;
    e.age += dt;
    switch (e.kind) {
    case EnemyKind::SEEKER:
    case EnemyKind::SPLITTER:
    case EnemyKind::SPLITTER_SHARD:
    case EnemyKind::MIRROR: {
        e.depth -= e.speed * dt;
        if (e.depth <= 0) {
            e.rimT += dt;
            if (e.lane == pl.lane && e.track == pl.track && pl.alive && pl.invulnT <= 0) {
                damagePlayer(audio, fx);
                e.alive = false;
            } else if (e.rimT > 1.6f) {
                e.alive = false; // retract
            }
        }
        break;
    }
    case EnemyKind::FLIPPER: {
        if (e.depth > 0) {
            e.depth -= e.speed * 0.8f * dt;
        } else {
            e.rimT += dt;
            e.flipT -= dt;
            if (e.flipT <= 0) {
                e.flipT = 0.3f + rng_.range(0.f, 0.35f);
                int dir = (pl.lane >= e.lane) ? 1 : -1;
                e.lane = (int)wrapF((float)(e.lane + dir), (float)P.lanes);
                audio.play(Sfx::FLIP, 1.f, 0.5f);
            }
            if (e.lane == pl.lane && e.track == pl.track && pl.alive && pl.invulnT <= 0) {
                damagePlayer(audio, fx);
                e.alive = false;
            }
        }
        break;
    }
    case EnemyKind::PULSAR: {
        if (e.depth > 0.3f) {
            e.depth -= e.speed * dt;
        } else {
            e.zapT -= dt;
            if (e.zapT <= 0) {
                e.zapT = 2.0f + rng_.range(0.f, 0.6f);
                e.zapping = true;
                e.zapHold = 0.5f;
                audio.play(Sfx::ZAP, 1.f, e.track == pl.track ? 1.f : 0.5f);
            }
            if (e.zapping) {
                e.zapHold -= dt;
                if (e.zapHold <= 0) e.zapping = false;
                else if (e.zapping && e.lane == pl.lane && e.track == pl.track &&
                         pl.alive && pl.invulnT <= 0)
                    damagePlayer(audio, fx);
            }
        }
        break;
    }
    case EnemyKind::WEAVER: {
        e.wob += dt * 2.5f;
        e.warpT += dt;
        e.warping = std::fmod(e.warpT, 2.6f) < 0.6f;
        e.depth -= e.speed * 0.7f * dt;
        if (e.depth <= 0) {
            e.rimT += dt;
            if (e.lane == pl.lane && e.track == pl.track && pl.alive && pl.invulnT <= 0) {
                damagePlayer(audio, fx);
                e.alive = false;
            } else if (e.rimT > 1.6f) {
                e.alive = false;
            }
        }
        break;
    }
    default: break;
    }
}

void World::update(float dt, const Intents& in, AudioEngine& audio, Fx& fx) {
    prevTime = time;
    time += dt;
    if (waveFlashT > 0) waveFlashT -= dt;
    if (superT > 0) superT -= dt;
    if (fireFlashT > 0) fireFlashT -= dt;
    if (noRingT > 0) noRingT -= dt;

    switch (phase) {
    case WorldPhase::INTRO:
        introT -= dt;
        // sector card is skippable: fire / start jumps straight into the action,
        // so the ship is never unresponsive after a sector starts
        if (introT <= 0 || in.fire || in.startPressed) {
            phase = WorldPhase::ACTIVE;
            if (introT > 0) introT = 0;
        }
        return;
    case WorldPhase::OUTRO:
        outroT -= dt;
        if (outroT <= 0)
            result = (P.level >= 99) ? WorldResult::LEVEL_CLEAR /*Game -> VICTORY*/ : WorldResult::LEVEL_CLEAR;
        return;
    default: break;
    }

    // ---- ACTIVE ----
    activeTime += dt;

    // wave indicator
    int wi = (int)(activeTime / P.waveGap);
    if (wi != waveIndex) {
        waveIndex = wi;
        waveFlashT = 1.2f;
    }

    // spawns
    while (spawnCursor < P.spawns.size() && P.spawns[spawnCursor].t <= activeTime) {
        const Spawn& s = P.spawns[spawnCursor++];
        spawn(s.kind, s.lane, s.track);
    }

    // threat level (drone intensity): deepest enemy on the player's track
    threat = 0;
    for (const Enemy& e : enemies)
        if (e.alive && e.track == pl.track)
            threat = std::max(threat, clamp01(1.f - e.depth / 0.6f));

    // player
    pl.prevLaneF = pl.laneF;
    if (!pl.alive) {
        pl.respawnT -= dt;
        if (pl.respawnT <= 0) {
            pl.alive = true;
            pl.invulnT = 2.f;
        }
    } else {
        pl.invulnT -= dt;
        float dir = mirrorActive() ? -1.f : 1.f;
        float rot = in.rotAxis + in.mouseDX * 0.02f;
        pl.laneF = wrapF(pl.laneF + dir * rot * 5.5f * dt, (float)P.lanes);
        pl.lane = ((int)std::floor(pl.laneF + 0.5f)) % P.lanes;

        pl.fireCd -= dt;
        if (in.fire && pl.fireCd <= 0) {
            Bullet b;
            b.lane = pl.lane; b.track = pl.track;
            b.depth = b.prevDepth = 0.02f;
            b.alive = true;
            bullets.push_back(b);
            pl.fireCd = 0.13f;
            fireFlashT = 0.07f;
            audio.play(Sfx::SHOOT, rng_.range(0.9f, 1.15f), 0.8f);
        }
        if (in.superPressed && supers > 0) {
            supers--;
            superT = 0.6f;
            fx.superWave();
            audio.play(Sfx::SUPER);
            for (Enemy& e : enemies)
                if (e.alive && e.depth < 0.85f) killEnemy(e, audio, fx, true);
        }
        if (in.ringPressed) {
            if (P.nested) {
                pl.track ^= 1;
                fx.warpPulse();
                audio.play(Sfx::WARP);
            } else {
                // denied: this sector has no second ring to jump to
                noRingT = 1.2f;
                audio.play(Sfx::MENU, 0.55f, 0.7f);
            }
        }
    }

    // bullets
    for (Bullet& b : bullets) {
        if (!b.alive) continue;
        b.prevDepth = b.depth;
        b.depth += 1.5f * dt;
        if (b.depth >= 1.f) b.alive = false;
    }

    // enemies
    for (Enemy& e : enemies)
        if (e.alive) updateEnemy(e, dt, audio, fx);

    // bullet vs enemy
    for (Bullet& b : bullets) {
        if (!b.alive) continue;
        for (Enemy& e : enemies) {
            if (!e.alive || e.track != b.track) continue;
            float el = e.lane + (e.kind == EnemyKind::WEAVER ? std::sin(e.wob) * 0.8f : 0);
            float d = wrapF((float)b.lane - el + P.lanes * 0.5f, (float)P.lanes) - P.lanes * 0.5f;
            if (std::fabs(d) < 0.55f && std::fabs(b.depth - e.depth) < 0.08f) {
                b.alive = false;
                if (e.kind == EnemyKind::WEAVER && e.warping) {
                    audio.play(Sfx::HIT); // deflect off the warp shimmer
                } else {
                    killEnemy(e, audio, fx, true);
                }
                break;
            }
        }
    }

    // deferred shard spawns (queued by killEnemy during iteration above)
    for (const DeferredSpawn& d : deferred_)
        spawn(d.kind, d.lane, d.track, d.depth, d.speedMul);
    deferred_.clear();

    // deferred arcade wipe (queued by damagePlayer)
    if (pendingClear_) {
        pendingClear_ = false;
        enemies.clear();
        bullets.clear();
        // skip spawns that expired during the wipe
        while (spawnCursor < P.spawns.size() && P.spawns[spawnCursor].t <= activeTime)
            spawnCursor++;
    }

    // cleanup
    for (size_t i = 0; i < enemies.size(); i++)
        if (!enemies[i].alive && enemies[i].age > 0.1f) {
            enemies[i] = enemies.back();
            enemies.pop_back();
            i--;
        }
    for (size_t i = 0; i < bullets.size(); i++)
        if (!bullets[i].alive) {
            bullets[i] = bullets.back();
            bullets.pop_back();
            i--;
        }

    // sector clear?
    if (spawnCursor >= P.spawns.size() && enemies.empty() && phase == WorldPhase::ACTIVE) {
        phase = WorldPhase::OUTRO;
        outroT = 2.2f;
        outroBonus = 500 * (P.level + 1);
        addBonus(outroBonus, audio);
    }
}

} // namespace nv
