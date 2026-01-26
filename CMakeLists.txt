cmake_minimum_required(VERSION 3.21)
project(neuro-vector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- SDL2: FetchContent pinned 2.30.8 by default (deterministic across all 3 OSes).
# Set -DNEURO_USE_SYSTEM_SDL=ON to use a system SDL2 (real SDL2, not sdl2-compat) ----
option(NEURO_USE_SYSTEM_SDL "Use system SDL2 instead of FetchContent" OFF)
set(NV_SDL_TARGET "")

if(NEURO_USE_SYSTEM_SDL)
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2)
        set(NV_SDL_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        set(NV_SDL_TARGET SDL2::SDL2-static)
    endif()
    if(NOT NV_SDL_TARGET)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2PC QUIET IMPORTED_TARGET sdl2)
            if(TARGET PkgConfig::SDL2PC)
                set(NV_SDL_TARGET PkgConfig::SDL2PC)
            endif()
        endif()
    endif()
endif()

if(NOT NV_SDL_TARGET)
    message(STATUS "Fetching SDL 2.30.8")
    include(FetchContent)
    FetchContent_Declare(sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-2.30.8.tar.gz)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sdl2)
    if(TARGET SDL2::SDL2)
        set(NV_SDL_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        set(NV_SDL_TARGET SDL2::SDL2-static)
    endif()
endif()

# ---- sources -----------------------------------------------------------------
add_executable(neuro-vector
    src/main.cpp
    src/selftest.cpp
    src/core/Game.cpp
    src/core/GameTest.cpp
    src/core/Config.cpp
    src/core/Input.cpp
    src/levels/LevelGen.cpp
    src/audio/AudioEngine.cpp
    src/renderer/VecFont.cpp
    src/renderer/CrtPass.cpp
    src/game/World.cpp
    src/game/RenderWorld.cpp
    src/fx/Fx.cpp
    src/meta/OperatorMenu.cpp
    src/meta/HelpScreen.cpp
)

target_include_directories(neuro-vector PRIVATE src)
target_link_libraries(neuro-vector PRIVATE ${NV_SDL_TARGET})
if(WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(neuro-vector PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(neuro-vector PRIVATE /W3 /permissive-)
else()
    target_compile_options(neuro-vector PRIVATE -Wall -Wextra)
endif()
