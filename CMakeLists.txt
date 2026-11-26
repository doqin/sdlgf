cmake_minimum_required(VERSION 3.24)

project(sdlgf
    VERSION 0.1.0
    DESCRIPTION "SDL3 + CMake port of the sdlgf game framework and demo"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json next to the build tree, for clangd and other IDE tooling
# (consumed by .clangd). Makefile/Ninja generators only.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SDLGF_TESTING "Define TESTING (demo boots into DebugScene)" OFF)
option(SDLGF_BUILD_TESTS "Build the unit tests (run with ctest)" ON)
option(SDLGF_COPY_ASSETS "Copy demo/assets next to the demo executable after building" ON)
option(SDLGF_D3DX_CROSSCHECK "Windows only: also compare the math library with the real D3DX9 (downloads the D3DX NuGet package)" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Dependencies)

add_subdirectory(core)
add_subdirectory(demo)

if(SDLGF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
