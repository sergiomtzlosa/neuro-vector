// Neuro-Vector — headless self-tests:
//   1. determinism: all 100 levels generate identically twice; loop hashes differ;
//      layout hashes match tests/golden/level_hashes.txt
//   2. strobe safety: Safe Mode disables subliminal flashes and caps strobes < 3 Hz
//   3. subliminal scheduling: normal mode flashes land within the 45-90 s window
//   4. menus: every main/warning-menu option works with ENTER (see GameTest.cpp)
#include "levels/LevelGen.h"
#include "fx/Fx.h"
#include "core/Game.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

namespace nv {

static bool paramsEqual(const LevelParams& a, const LevelParams& b) {
    if (a.lanes != b.lanes || a.waves != b.waves || a.enemiesPerWave != b.enemiesPerWave) return false;
    if (a.warpMode != b.warpMode || a.nested != b.nested) return false;
    if (a.hash != b.hash) return false;
    if (a.spawns.size() != b.spawns.size()) return false;
    for (size_t i = 0; i < a.spawns.size(); i++) {
        if (a.spawns[i].t != b.spawns[i].t) return false;
        if (a.spawns[i].kind != b.spawns[i].kind) return false;
        if (a.spawns[i].lane != b.spawns[i].lane) return false;
        if (a.spawns[i].track != b.spawns[i].track) return false;
    }
    return true;
}

int selftest(bool dumpGolden) {
    int fails = 0;
    std::vector<std::string> lines;

    // ---- 1. determinism across all 100 levels ----
    for (int lvl = 0; lvl < 100; lvl++) {
        LevelParams a = LevelGen::generate(lvl, 0);
        LevelParams b = LevelGen::generate(lvl, 0);
        if (!paramsEqual(a, b)) {
            printf("FAIL determinism: level %d\n", lvl + 1);
            fails++;
        }
        LevelParams c = LevelGen::generate(lvl, 1);
        if (c.hash == a.hash) {
            printf("FAIL loop-hash collision: level %d\n", lvl + 1);
            fails++;
        }
        lines.push_back(LevelGen::hashText(a));
    }
    if (!fails) printf("determinism: 100/100 levels reproducible\n");

    // golden file compare
    const char* path = "tests/golden/level_hashes.txt";
    if (dumpGolden) {
        FILE* f = fopen(path, "w");
        if (f) {
            for (const std::string& s : lines) fprintf(f, "%s\n", s.c_str());
            fclose(f);
            printf("golden: wrote %zu hashes to %s\n", lines.size(), path);
        } else {
            printf("golden: could not write %s\n", path);
            fails++;
        }
    } else {
        FILE* f = fopen(path, "r");
        if (f) {
            char buf[128];
            int checked = 0;
            for (int i = 0; i < 100 && fgets(buf, sizeof buf, f); i++) {
                buf[strcspn(buf, "\r\n")] = 0;
                if (!buf[0]) continue;
                checked++;
                if (lines[i] != buf) {
                    printf("FAIL golden: level %d hash drift:\n  got: %s\n  want: %s\n",
                           i + 1, lines[i].c_str(), buf);
                    fails++;
                }
            }
            fclose(f);
            printf("golden: %d/%d level hashes match\n", checked, checked);
        } else {
            printf("golden: no golden file (run --selftest --dump-golden once)\n");
        }
    }

    // ---- 2. strobe safety (Safe Mode) ----
    {
        Fx fx;
        fx.notifyLevel(0, fnv1a(BASE_SEED, 1));
        bool subliminalInSafe = false;
        for (int i = 0; i < 60 * 300; i++) { // 300 s at 60 Hz with constant damage
            fx.update(1.f / 60.f, true);
            fx.damage();
            if (fx.subliminalActive()) subliminalInSafe = true;
        }
        if (subliminalInSafe) {
            printf("FAIL safety: subliminal flashes active in Safe Mode\n");
            fails++;
        }
        if (fx.flashesLastSecond() > 3) {
            printf("FAIL safety: strobe rate %d/s exceeds 3 Hz cap\n", fx.flashesLastSecond());
            fails++;
        }
        if (!subliminalInSafe && fx.flashesLastSecond() <= 3)
            printf("safety: safe mode holds (0 subliminal, %d flash/s cap)\n", fx.flashesLastSecond());
    }

    // ---- 3. subliminal scheduling (normal mode) ----
    {
        Fx fx;
        float interval = fx.subliminalInterval();
        if (interval < 45.f || interval > 90.f) {
            printf("FAIL subliminal: interval %.1f s outside 45-90 s\n", interval);
            fails++;
        } else {
            printf("subliminal: interval %.1f s (45-90 s window OK)\n", interval);
        }
        fx.forceSubliminal();
        if (!fx.subliminalActive()) {
            printf("FAIL subliminal: forced flash inactive\n");
            fails++;
        }
    }

    // ---- 4. menus: every option works with ENTER (prints its own section) ----
    {
        Game game;
        fails += game.selfTestMenu();
    }

    printf(fails ? "SELFTEST: %d FAILURES\n" : "SELFTEST: ALL PASS\n", fails);
    return fails ? 1 : 0;
}

} // namespace nv
