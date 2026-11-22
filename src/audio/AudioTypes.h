// Neuro-Vector — SFX identifiers shared between game thread and audio thread.
#pragma once

namespace nv {

enum class Sfx : int {
    SHOOT,      // player laser
    HIT,        // enemy hit but survived (weaver warp)
    SHARD,      // splitter split
    DEATH,      // enemy destroyed
    FLIP,       // flipper lane flip
    ZAP,        // pulsar electrifies lane
    MIRROR,     // mirror spawn aura
    WARP,       // player ring jump
    PLAYER_HIT, // player lost a life
    SUPER,      // smart bomb
    LEVEL_START,// sector intro riser
    LEVEL_CLEAR,// sector fanfare
    EXTRA,      // extra life / super charge
    MENU,       // menu blip
    COIN,       // coin inserted
    GAMEOVER,   // descending game-over tone
    COUNT
};

struct SfxEvent {
    Sfx   id;
    float pitch; // 1 = nominal
    float vol;   // 0..1 relative
};

} // namespace nv
