#include "Config.h"
#include <cstdio>
#include <cstring>
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace nv {

static std::string prefDir() {
    char* p = SDL_GetPrefPath("neuro-vector", "");
    std::string d = p ? p : "./";
    SDL_free(p);
    return d;
}

void SaveData::load() {
    dir = prefDir();
    count = 0;

    if (FILE* f = fopen((dir + "settings.cfg").c_str(), "r")) {
        char line[256];
        while (fgets(line, sizeof line, f)) {
            char key[64]; float v = 0;
            if (sscanf(line, " %63[^=]= %f", key, &v) != 2) continue;
            bool on = v > 0.5f;
            if      (!strcmp(key, "safeMode"))  settings.safeMode = on;
            else if (!strcmp(key, "crtFx"))     settings.crtFx = on;
            else if (!strcmp(key, "mouseDial")) settings.mouseDial = on;
            else if (!strcmp(key, "fullscreen"))settings.fullscreen = on;
            else if (!strcmp(key, "volMaster")) settings.volMaster = v;
            else if (!strcmp(key, "volMusic"))  settings.volMusic = v;
            else if (!strcmp(key, "volSfx"))    settings.volSfx = v;
            else if (!strcmp(key, "volDrone"))  settings.volDrone = v;
        }
        fclose(f);
    }
    if (FILE* f = fopen((dir + "scores.dat").c_str(), "r")) {
        char line[128];
        while (count < 5 && fgets(line, sizeof line, f)) {
            char ini[8] = "...";
            int sc = 0;
            if (sscanf(line, " %d %7s", &sc, ini) >= 1) {
                scores[count].score = sc;
                strncpy(scores[count].initials, ini, 3);
                scores[count].initials[3] = 0;
                count++;
            }
        }
        fclose(f);
    }
}

void SaveData::save() {
    if (dir.empty()) dir = prefDir();
    if (FILE* f = fopen((dir + "settings.cfg").c_str(), "w")) {
        fprintf(f, "safeMode = %d\ncrtFx = %d\nmouseDial = %d\nfullscreen = %d\n",
                settings.safeMode ? 1 : 0, settings.crtFx ? 1 : 0,
                settings.mouseDial ? 1 : 0, settings.fullscreen ? 1 : 0);
        fprintf(f, "volMaster = %.3f\nvolMusic = %.3f\nvolSfx = %.3f\nvolDrone = %.3f\n",
                settings.volMaster, settings.volMusic, settings.volSfx, settings.volDrone);
        fclose(f);
    }
    if (FILE* f = fopen((dir + "scores.dat").c_str(), "w")) {
        for (int i = 0; i < count; i++)
            fprintf(f, "%d %s\n", scores[i].score, scores[i].initials);
        fclose(f);
    }
}

bool SaveData::qualifies(int score) const {
    if (score <= 0) return false;
    return count < 5 || score > scores[count - 1].score;
}

int SaveData::insert(int score, const char* initials) {
    if (!qualifies(score)) return -1;
    int rank = 0;
    while (rank < count && scores[rank].score >= score) rank++;
    if (count < 5) count++;
    for (int i = count - 1; i > rank; i--) scores[i] = scores[i - 1];
    scores[rank].score = score;
    strncpy(scores[rank].initials, initials, 3);
    scores[rank].initials[3] = 0;
    save();
    return rank;
}

void SaveData::resetScores() { count = 0; save(); }

} // namespace nv
