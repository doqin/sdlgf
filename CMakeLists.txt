cmake_minimum_required(VERSION 3.24)

project(sdlgf
    VERSION 0.1.0
    DESCRIPTION "SDL3 + CMake port of the sdlgf game framework and demo"
    LANGUAGES CXX
)

# Phase 1 of the port: the existing Direct3D 9 / DirectInput / XAudio2 code still
# builds as-is, on Windows only. Later phases replace those pieces with SDL3 and
# this guard goes away.
if(NOT WIN32)
    message(FATAL_ERROR
        "sdlgf is mid-port: the code still depends on Win32/DirectX and only "
        "builds on Windows for now.")
endif()
enable_language(RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SDLGF_TESTING "Define TESTING (demo boots into DebugScene)" OFF)
option(SDLGF_BUILD_TESTS "Build the unit tests (run with ctest)" ON)
option(SDLGF_COPY_ASSETS "Copy demo/assets next to the demo executable after building" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Dependencies)

add_subdirectory(core)
add_subdirectory(demo)

# Golden-image generator for the legacy Direct3D 9 renderer (removed together with it).
option(SDLGF_BUILD_GOLDEN_GEN "Build tools/golden (Windows only, needs the D3D9 renderer)" OFF)
if(SDLGF_BUILD_GOLDEN_GEN AND WIN32)
    add_subdirectory(tools/golden)
endif()

if(SDLGF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
