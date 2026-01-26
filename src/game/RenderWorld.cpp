#include "RenderWorld.h"
#include "World.h"
#include "Types.h"
#include "../levels/LevelGen.h"
#include "../renderer/VecFont.h"
#include <cmath>

namespace nv {

Palette makePalette(const LevelParams& P) {
    Palette pal;
    float h = P.hue;
    pal.tunnel = hsl(h, 0.55f, 0.55f);
    pal.lane   = withA(hsl(h + 0.02f, 0.4f, 0.45f), 0.8f);
    pal.rim    = hsl(h + 0.5f, 0.7f, 0.65f);
    pal.player = hsl(h + 0.5f, 0.15f, 0.92f);
    pal.zap    = hsl(h + 0.83f, 0.9f, 0.7f);
    pal.ui     = hsl(h + 0.5f, 0.25f, 0.85f);
    pal.dim    = withA(pal.tunnel, 0.35f);
    float eh[8] = {0.55f, 0.08f, 0.33f, 0.33f, 0.83f, 0.66f, 0.44f, 0.5f};
    for (int i = 0; i < 8; i++)
        pal.enemy[i] = hsl(h + eh[i], 0.85f, 0.62f);
    return pal;
}

// ---- projection ------------------------------------------------------------

struct View {
    float cx, cy, R;
    float spin, roll;
    int lanes;
};

static float warpFactor(float ang, const LevelParams& P) {
    // Regular polygon boundary: vertices sit on the circle at lane angles,
    // edges dip inside — entities sliding between lanes stay ON the rim.
    float seg = TAU / (float)P.lanes;
    float d = std::fmod(ang, seg);
    if (d < 0) d += seg;
    float delta = std::fabs(d - seg * 0.5f);
    float poly = std::cos(seg * 0.5f) / std::cos(std::min(delta, seg * 0.5f - 0.001f));
    if (P.tier == 0 || P.warpMode == 0) return poly;
    float k = P.warpK;
    float p = P.warpP;
    float a = ang + P.warpPh;
    float warp = 1.f;
    switch (P.warpMode) {
    case 1: { // superellipse grid
        float pp = 2.f + p * 2.f;
        float r = std::pow(std::pow(std::fabs(std::cos(a)), pp) + std::pow(std::fabs(std::sin(a)), pp), -1.f / pp);
        warp = r * 1.12f;
        break;
    }
    case 2:   // lobed lemniscate family
        warp = 0.62f + 0.38f * std::pow(std::fabs(std::cos(k * a * 0.5f)), p);
        break;
    default:  // infinity pinch
        warp = 0.55f + 0.45f * std::sqrt(std::fabs(std::cos(k * a)));
        break;
    }
    return poly * warp;
}

static Vec2 rotAround(Vec2 p, Vec2 c, float a) {
    float s = std::sin(a), co = std::cos(a);
    Vec2 d = p - c;
    return c + Vec2{d.x * co - d.y * s, d.x * s + d.y * co};
}

struct Proj {
    Vec2 pos;
    float scale;
};

static Proj project(const World& w, const View& v, float laneF, int track, float depth) {
    float ang = TAU * laneF / (float)v.lanes + v.spin;
    float wf = warpFactor(ang, w.P);
    float r = v.R * wf * ((w.P.nested && track == 1) ? 0.45f : 1.f);
    Vec2 c{v.cx, v.cy};
    Vec2 rim = rotAround(c + Vec2{std::cos(ang) * r, std::sin(ang) * r}, c, v.roll);
    Vec2 inner = c + (rim - c) * 0.12f;
    float t = std::pow(clamp01(depth), 0.85f);
    Proj out;
    out.pos = rim + (inner - rim) * t;
    out.scale = lerp(1.f, 0.28f, t);
    return out;
}

// ---- enemy shapes ----------------------------------------------------------

static void drawEnemyShape(LineRenderer& lr, EnemyKind kind, Vec2 p, float s, Color c,
                           float t, bool warping, int glow);

// public: draw one enemy archetype at p (used by the help-screen bestiary)
void drawEnemyGlyph(LineRenderer& lr, int kind, Vec2 p, float s, Color c, float t) {
    drawEnemyShape(lr, (EnemyKind)kind, p, s, c, t, false, 0);
}

// vector spaceship icon pointing +x (HUD lives row)
void drawShipIcon(LineRenderer& lr, Vec2 p, float s, Color c) {
    Vec2 nose{p.x + s, p.y};
    Vec2 hr{p.x + s * 0.15f, p.y - s * 0.34f};
    Vec2 hl{p.x + s * 0.15f, p.y + s * 0.34f};
    Vec2 wr{p.x - s * 0.45f, p.y - s * 0.85f};
    Vec2 wl{p.x - s * 0.45f, p.y + s * 0.85f};
    Vec2 tail{p.x - s * 0.55f, p.y};
    lr.line(nose.x, nose.y, hr.x, hr.y, c, 1);
    lr.line(hr.x, hr.y, wr.x, wr.y, c, 1);
    lr.line(wr.x, wr.y, tail.x, tail.y, c, 1);
    lr.line(tail.x, tail.y, wl.x, wl.y, c, 1);
    lr.line(wl.x, wl.y, hl.x, hl.y, c, 1);
    lr.line(hl.x, hl.y, nose.x, nose.y, c, 1);
}

static void drawEnemyShape(LineRenderer& lr, EnemyKind kind, Vec2 p, float s, Color c,
                           float t, bool warping, int glow) {
    if (warping) {
        c = withA(c, 0.4f + 0.3f * std::sin(t * 30.f));
        s *= 1.15f;
    }
    Vec2 pts[8];
    int n = 0;
    switch (kind) {
    case EnemyKind::SEEKER: // diamond
        pts[0] = {p.x, p.y - s}; pts[1] = {p.x + s * 0.7f, p.y};
        pts[2] = {p.x, p.y + s}; pts[3] = {p.x - s * 0.7f, p.y};
        n = 4;
        lr.poly(pts, n, c, glow);
        break;
    case EnemyKind::FLIPPER: // bowtie X
        pts[0] = {p.x + s * 0.8f, p.y - s * 0.8f}; pts[1] = {p.x + s * 0.8f, p.y + s * 0.8f};
        pts[2] = {p.x - s * 0.8f, p.y - s * 0.8f}; pts[3] = {p.x - s * 0.8f, p.y + s * 0.8f};
        n = 4;
        lr.poly(pts, n, c, glow);
        break;
    case EnemyKind::SPLITTER: { // hexagon
        for (int i = 0; i < 6; i++) {
            float a = TAU * i / 6.f + t * 1.5f;
            pts[i] = {p.x + std::cos(a) * s * 0.85f, p.y + std::sin(a) * s * 0.85f};
        }
        n = 6;
        lr.poly(pts, n, c, glow);
        lr.line(p.x - s * 0.4f, p.y, p.x + s * 0.4f, p.y, c, glow);
        break;
    }
    case EnemyKind::SPLITTER_SHARD: // small triangle
        pts[0] = {p.x, p.y - s * 0.6f}; pts[1] = {p.x + s * 0.6f, p.y + s * 0.4f};
        pts[2] = {p.x - s * 0.6f, p.y + s * 0.4f};
        n = 3;
        lr.poly(pts, n, c, glow);
        break;
    case EnemyKind::PULSAR: { // circle + spokes
        for (int i = 0; i < 8; i++) {
            float a = TAU * i / 8.f;
            pts[i] = {p.x + std::cos(a) * s * 0.8f, p.y + std::sin(a) * s * 0.8f};
        }
        n = 8;
        lr.poly(pts, n, c, glow);
        lr.line(p.x - s, p.y, p.x + s, p.y, c, glow);
        lr.line(p.x, p.y - s, p.x, p.y + s, c, glow);
        break;
    }
    case EnemyKind::MIRROR: { // hourglass
        pts[0] = {p.x + s * 0.8f, p.y - s * 0.9f}; pts[1] = {p.x + s * 0.8f, p.y + s * 0.9f};
        pts[2] = {p.x, p.y}; pts[3] = {p.x - s * 0.8f, p.y + s * 0.9f};
        pts[4] = {p.x - s * 0.8f, p.y - s * 0.9f}; pts[5] = {p.x, p.y};
        n = 6;
        lr.poly(pts, n, c, glow);
        break;
    }
    case EnemyKind::WEAVER: // chevrons
        pts[0] = {p.x + s * 0.8f, p.y - s * 0.9f};
        pts[1] = {p.x - s * 0.8f, p.y - s * 0.2f};
        pts[2] = {p.x + s * 0.8f, p.y + s * 0.2f};
        pts[3] = {p.x - s * 0.8f, p.y + s * 0.9f};
        n = 4;
        lr.poly(pts, n, c, glow, false);
        break;
    default: break;
    }
}

// ---- world -----------------------------------------------------------------

void drawWorld(LineRenderer& lr, const World& w, const Palette& pal,
               float width, float height, float alpha, float shakeX, float shakeY) {
    const LevelParams& P = w.P;
    View v;
    v.cx = width * 0.5f + shakeX;
    v.cy = height * 0.5f + shakeY;
    // fit the tunnel inside the screen: the superellipse warp overshoots ~12%
    // and the beat pulse grows the rim, so pre-divide the base radius — the
    // tunnel pumps up to (never past) the fit circle
    float maxWarp = (P.tier > 0 && P.warpMode == 1) ? 1.12f : 1.f;
    v.R = std::min(width, height) * 0.40f / (maxWarp * (1.f + P.pulse));
    if (P.nested) v.R *= 0.92f; // hyper sectors: keep the dual rings in frame
    float t = lerp(w.prevTime, w.time, alpha);
    v.spin = P.spin * t;
    v.roll = w.camRollAt(t);
    v.lanes = P.lanes;
    if (w.P.nested) v.roll += std::sin(t * 3.1f) * 0.02f;

    // tunnel rings
    for (int k = 0; k <= 10; k++) {
        float d = k / 10.f;
        for (int track = 0; track < (P.nested ? 2 : 1); track++) {
            Vec2 ring[17];
            for (int i = 0; i < P.lanes; i++) {
                Proj pj = project(w, v, (float)i, track, d);
                ring[i] = pj.pos;
            }
            float dd = std::fabs(d - 0.f);
            Color c = (dd < 0.01f) ? pal.rim : withA(pal.tunnel, 0.85f - 0.45f * d);
            lr.poly(ring, P.lanes, c, 0);
        }
    }

    // lane spokes
    for (int i = 0; i < P.lanes; i++) {
        for (int track = 0; track < (P.nested ? 2 : 1); track++) {
            Proj a = project(w, v, (float)i, track, 0.f);
            Proj b = project(w, v, (float)i, track, 1.f);
            bool playerHere = (i == w.pl.lane && track == w.pl.track && w.pl.alive);
            Color c = playerHere ? withA(pal.player, 0.5f) : pal.lane;
            lr.line(a.pos.x, a.pos.y, b.pos.x, b.pos.y, c, playerHere ? 1 : 0);
        }
    }

    // pulsar zaps: electrified lanes
    for (const Enemy& e : w.enemies) {
        if (!e.alive || !e.zapping) continue;
        Vec2 prev;
        for (int k = 0; k <= 12; k++) {
            float d = k / 12.f;
            Proj pj = project(w, v, (float)e.lane, e.track, d);
            float jit = std::sin(t * 40.f + k * 2.3f + e.age * 7.f) * 5.f * pj.scale;
            Vec2 p = pj.pos + Vec2{jit, jit * 0.6f};
            if (k > 0) lr.line(prev.x, prev.y, p.x, p.y, pal.zap, 2);
            prev = p;
        }
    }

    // bullets
    for (const Bullet& b : w.bullets) {
        if (!b.alive) continue;
        Proj a = project(w, v, (float)b.lane, b.track, std::max(0.f, b.depth - 0.09f));
        Proj c = project(w, v, (float)b.lane, b.track, b.depth);
        lr.line(a.pos.x, a.pos.y, c.pos.x, c.pos.y, withA(pal.player, 0.95f), 2);
        lr.line(c.pos.x, c.pos.y, c.pos.x, c.pos.y, rgba(255, 255, 255, 220), 0);
    }

    // enemies
    float base = v.R * 0.085f;
    for (const Enemy& e : w.enemies) {
        if (!e.alive) continue;
        float el = e.lane + (e.kind == EnemyKind::WEAVER ? std::sin(e.wob) * 0.8f : 0);
        float d = lerp(e.prevDepth, e.depth, alpha);
        Proj pj = project(w, v, el, e.track, d);
        float s = base * (0.35f + 0.65f * (1.f - clamp01(d)));
        Color c = pal.enemy[(int)e.kind];
        int glow = d < 0.25f ? 2 : 1;
        drawEnemyShape(lr, e.kind, pj.pos, s * pj.scale, c, t, e.warping, glow);
    }

    // player spaceship: hull, swept wings, cockpit, engine flame — nose points
    // inward, the direction of fire
    if (w.pl.alive) {
        float laneF = lerp(w.pl.prevLaneF, w.pl.laneF, alpha);
        Proj pj = project(w, v, laneF, w.pl.track, 0.f);
        Vec2 c{v.cx, v.cy};
        Vec2 fwd = c - pj.pos;
        float fl = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
        if (fl > 1.f) fwd = fwd * (1.f / fl);
        Vec2 side{-fwd.y, fwd.x};
        float s = v.R * 0.085f;
        float blink = w.pl.invulnT > 0 ? 0.35f + 0.65f * std::fabs(std::sin(t * 12.f)) : 1.f;
        Color pc = withA(pal.player, blink);
        auto at = [&](float lx, float ly) {
            return pj.pos + fwd * (lx * s) + side * (ly * s);
        };
        Vec2 nose = at(1.0f, 0);
        Vec2 hr = at(0.15f, 0.30f), hl = at(0.15f, -0.30f);
        Vec2 wr = at(-0.45f, 0.82f), wl = at(-0.45f, -0.82f);
        Vec2 br = at(-0.70f, 0.48f), bl = at(-0.70f, -0.48f);
        Vec2 tail = at(-0.55f, 0);
        lr.line(nose.x, nose.y, hr.x, hr.y, pc, 2);
        lr.line(hr.x, hr.y, wr.x, wr.y, pc, 2);
        lr.line(wr.x, wr.y, br.x, br.y, pc, 2);
        lr.line(br.x, br.y, tail.x, tail.y, pc, 2);
        lr.line(tail.x, tail.y, bl.x, bl.y, pc, 2);
        lr.line(bl.x, bl.y, wl.x, wl.y, pc, 2);
        lr.line(wl.x, wl.y, hl.x, hl.y, pc, 2);
        lr.line(hl.x, hl.y, nose.x, nose.y, pc, 2);
        // cockpit
        Vec2 cp[3] = {at(0.52f, 0), at(0.18f, 0.13f), at(0.18f, -0.13f)};
        lr.poly(cp, 3, withA(pal.player, blink * 0.8f), 1);
        // engine flame (flicker) + muzzle flash on fire
        float flick = 0.5f + 0.5f * std::sin(t * 31.f);
        float boost = w.fireFlashT > 0 ? w.fireFlashT / 0.07f : 0.f;
        Color fc = rgba(255, 160, 60, (int)(blink * (200 + 55 * boost)));
        Vec2 e0 = at(-0.62f, 0), e1 = at(-0.95f - 0.4f * flick - 0.5f * boost, 0);
        lr.line(e0.x, e0.y, e1.x, e1.y, fc, 1 + (boost > 0.4f ? 1 : 0));
        lr.line(at(-0.62f, 0.14f).x, at(-0.62f, 0.14f).y, e1.x, e1.y, withA(fc, 0.6f), 0);
        lr.line(at(-0.62f, -0.14f).x, at(-0.62f, -0.14f).y, e1.x, e1.y, withA(fc, 0.6f), 0);
        // muzzle star at the nose while firing
        if (boost > 0.f) {
            float mr = s * (0.28f + 0.55f * boost);
            lr.line(nose.x - mr, nose.y, nose.x + mr, nose.y, rgba(255, 220, 150, (int)(230 * boost)), 1);
            lr.line(nose.x, nose.y - mr, nose.x, nose.y + mr, rgba(255, 220, 150, (int)(230 * boost)), 1);
        }
    }

    // smart-bomb ring
    if (w.superT > 0) {
        float rr = v.R * (1.f - w.superT / 0.6f) + 10.f;
        Vec2 ring[24];
        for (int i = 0; i < 24; i++) {
            float a = TAU * i / 24.f;
            ring[i] = {v.cx + std::cos(a) * rr, v.cy + std::sin(a) * rr};
        }
        lr.poly(ring, 24, withA(pal.player, w.superT / 0.6f), 2);
    }
}

// ---- HUD -------------------------------------------------------------------

void drawHud(LineRenderer& lr, const World& w, float width, float height, float blink, bool demo) {
    const LevelParams& P = w.P;
    Palette pal = makePalette(P);
    char buf[96];

    float margin = width * 0.055f;
    float hs = height * 0.034f;

    // score (top right)
    snprintf(buf, sizeof buf, "%07d", w.score);
    VecFont::draw(lr, buf, width - margin, margin + hs, hs, pal.ui, VecFont::RIGHT, 1);
    snprintf(buf, sizeof buf, "x%d", w.mult);
    VecFont::draw(lr, buf, width - margin, margin + hs * 2.4f, hs * 0.8f,
                  w.mult > 1 ? pal.rim : withA(pal.ui, 0.5f), VecFont::RIGHT, 1);

    // sector (top left)
    snprintf(buf, sizeof buf, "SECTOR %02d", P.level + 1);
    VecFont::draw(lr, buf, margin, margin + hs, hs, pal.ui, VecFont::LEFT, 1);
    if (P.loop > 0) {
        snprintf(buf, sizeof buf, "LOOP %d", P.loop);
        VecFont::draw(lr, buf, margin, margin + hs * 2.1f, hs * 0.7f, pal.rim, VecFont::LEFT, 1);
    }

    // lives (bottom left): one small spaceship per remaining ship
    float ls = height * 0.028f;
    for (int i = 0; i < std::max(0, w.lives); i++)
        drawShipIcon(lr, {margin + i * ls * 2.6f, height - margin}, ls, pal.player);

    // super charges (bottom right)
    for (int i = 0; i < w.supers; i++) {
        float x = width - margin - i * hs * 1.6f;
        Vec2 d[4] = {{x, height - margin - hs * 0.5f}, {x + hs * 0.4f, height - margin},
                     {x, height - margin + hs * 0.5f}, {x - hs * 0.4f, height - margin}};
        lr.poly(d, 4, pal.rim, 1);
    }

    // wave flash (top center, between SECTOR and the score)
    if (w.waveFlashT > 0 && w.phase == WorldPhase::ACTIVE) {
        snprintf(buf, sizeof buf, "WAVE %d", w.waveIndex + 1);
        Color c = withA(pal.ui, clamp01(w.waveFlashT));
        VecFont::draw(lr, buf, width * 0.5f, margin + hs, hs * 0.9f, c, VecFont::CENTER, 1);
    }

    if (demo) {
        if (std::fmod(blink, 1.f) < 0.6f)
            VecFont::draw(lr, "DEMO", width * 0.5f, margin + hs * 2.2f, hs, withA(pal.rim, 0.9f), VecFont::CENTER, 1);
    }

    // ring-jump denied: this sector has no second ring to jump to
    if (w.noRingT > 0 && w.phase == WorldPhase::ACTIVE) {
        snprintf(buf, sizeof buf, "RING JUMP: HYPER SECTORS (67+) ONLY");
        Color c = rgba(255, 120, 70, (int)(255 * clamp01(w.noRingT)));
        VecFont::draw(lr, buf, width * 0.5f, margin + hs * 2.2f, hs * 0.8f, c, VecFont::CENTER, 1);
    }

    // intro card
    if (w.phase == WorldPhase::INTRO) {
        float a = clamp01(w.introT / 0.4f);
        snprintf(buf, sizeof buf, "WARP SECTOR %02d", P.level + 1);
        VecFont::draw(lr, buf, width * 0.5f, height * 0.42f, height * 0.075f,
                      withA(pal.ui, a), VecFont::CENTER, 2);
        VecFont::draw(lr, P.geoName.c_str(), width * 0.5f, height * 0.52f, height * 0.042f,
                      withA(pal.rim, a), VecFont::CENTER, 1);
        if (P.loop > 0) {
            snprintf(buf, sizeof buf, "LOOP %d", P.loop);
            VecFont::draw(lr, buf, width * 0.5f, height * 0.58f, height * 0.03f,
                          withA(pal.rim, a), VecFont::CENTER, 1);
        }
    }

    // outro card
    if (w.phase == WorldPhase::OUTRO) {
        float a = clamp01(w.outroT / 0.4f);
        VecFont::draw(lr, "SECTOR CLEAR", width * 0.5f, height * 0.42f, height * 0.055f,
                      withA(pal.ui, a), VecFont::CENTER, 2);
        snprintf(buf, sizeof buf, "BONUS %d", (int)w.outroBonus);
        VecFont::draw(lr, buf, width * 0.5f, height * 0.5f, height * 0.04f,
                      withA(pal.rim, a), VecFont::CENTER, 1);
    }
}

} // namespace nv
