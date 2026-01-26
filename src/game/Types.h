// Neuro-Vector — shared gameplay types.
#pragma once
#include <cstdint>

namespace nv {

enum class EnemyKind : int {
    SEEKER = 0, FLIPPER, SPLITTER, SPLITTER_SHARD, PULSAR, MIRROR, WEAVER, COUNT
};

// Base stats per archetype: score, lane-crossing speed (depth/sec), behaviors tune in World.
struct KindStats {
    float score;
    float speed;
};

constexpr KindStats kindStats[(int)EnemyKind::COUNT] = {
    /*SEEKER*/        { 50, 0.155f },
    /*FLIPPER*/       { 60, 0.130f },
    /*SPLITTER*/      { 40, 0.140f },
    /*SPLITTER_SHARD*/{ 30, 0.320f },
    /*PULSAR*/        { 70, 0.150f },
    /*MIRROR*/        { 80, 0.120f },
    /*WEAVER*/        { 90, 0.110f },
};

} // namespace nv
