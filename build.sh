#!/bin/sh
# Neuro-Vector — build / clean / run / test script.
#
# Usage: ./build.sh [build|clean|run|test|debug]
#   (no arg)  configure (Release) + build
#   build     configure + build (Release)
#   debug     configure + build (Debug)
#   clean     remove the build directory
#   run       build, then launch the game
#   test      build, then run the headless selftest
set -e
cd "$(dirname "$0")"

BUILD_DIR=build
TYPE=Release

cmd="${1:-build}"
if [ "$cmd" = "debug" ]; then TYPE=Debug; fi

case "$cmd" in
clean)
    rm -rf "$BUILD_DIR"
    echo "cleaned ./$BUILD_DIR"
    ;;

build|run|test|debug|release)
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$cmd" = "debug" ]; then
        echo "== configuring ($TYPE) =="
        cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$TYPE"
    fi
    echo "== building =="
    cmake --build "$BUILD_DIR" -j
    if [ "$cmd" = "test" ]; then
        echo "== selftest =="
        ./"$BUILD_DIR"/neuro-vector --selftest
    elif [ "$cmd" = "run" ]; then
        exec ./"$BUILD_DIR"/neuro-vector
    else
        echo "== done: $BUILD_DIR/neuro-vector =="
    fi
    ;;

*)
    echo "usage: ./build.sh [build|clean|run|test|debug]"
    exit 1
    ;;
esac
