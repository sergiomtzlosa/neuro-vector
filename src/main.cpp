// Neuro-Vector — entry point.
// Flags: --selftest [--dump-golden] | --menutest | --shot FILE [--level N] [--frames F] [--safe]
#include "core/Game.h"
#include <cstring>
#include <cstdlib>

namespace nv {
int selftest(bool dumpGolden);
}

int main(int argc, char** argv) {
    nv::Game game;
    const char* shotPath = nullptr;
    int level = 0, frames = 260;
    bool safe = false, selftest = false, dump = false, menutest = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--selftest")) selftest = true;
        else if (!strcmp(argv[i], "--dump-golden")) dump = true;
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc) shotPath = argv[++i];
        else if (!strcmp(argv[i], "--menutest")) menutest = true;
        else if (!strcmp(argv[i], "--level") && i + 1 < argc) level = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--safe")) safe = true;
    }
    if (selftest) return nv::selftest(dump);
    if (menutest) return game.selfTestMenu();
    if (shotPath) return game.shot(shotPath, level, frames, safe);
    return game.run(argc, argv);
}
