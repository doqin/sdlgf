cmake_minimum_required(VERSION 3.24)

project(sdlgf
    VERSION 0.1.0
    DESCRIPTION "SDL3 + CMake port of the sdlgf game framework and demo"
    LANGUAGES CXX
)

# Mid-port: window, input and rendering are SDL3 now, but audio is still XAudio2, so
# the build is Windows-only until that is replaced; this guard goes away then.
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
