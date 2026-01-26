// Neuro-Vector — settings + high-score persistence (portable, no binary assets).
#pragma once
#include <string>
#include <vector>

namespace nv {

struct Settings {
    bool safeMode   = false;
    bool crtFx      = true;
    bool mouseDial  = true;
    bool fullscreen = false;
    float volMaster = 0.8f;
    float volMusic  = 0.65f;
    float volSfx    = 0.9f;
    float volDrone  = 0.5f;
};

struct ScoreEntry {
    int  score = 0;
    char initials[4] = "...";
};

class SaveData {
public:
    Settings   settings;
    ScoreEntry scores[5];
    int        count = 0;
    std::string dir; // pref path (set on load)

    void load();
    void save();
    bool qualifies(int score) const;
    // Returns rank (0-based) if inserted, -1 otherwise.
    int insert(int score, const char* initials);
    void resetScores();
};

} // namespace nv
