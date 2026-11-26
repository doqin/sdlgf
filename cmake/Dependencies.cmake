# Third-party dependencies, fetched and built from source (no prebuilt binaries
# are checked into the repo).
include(FetchContent)

# Some vendored projects still declare cmake_minimum_required(VERSION < 3.5),
# which CMake 4 rejects unless told otherwise.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# --- SDL3 -------------------------------------------------------------------
# Not used by the game code yet (phase 3+). Fetched now so the build proves SDL3
# compiles on every platform we care about.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.20
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

# --- tmxlite ----------------------------------------------------------------
# Pinned to the upstream commit whose headers match the ones that used to be
# vendored under includes/tmxlite (2026-01-15, "Add documentation note for
# animation frame").
set(TMXLITE_STATIC_LIB TRUE CACHE BOOL "" FORCE)
set(USE_EXTLIBS FALSE CACHE BOOL "" FORCE)
FetchContent_Declare(tmxlite
    GIT_REPOSITORY https://github.com/fallahn/tmxlite.git
    GIT_TAG        b308fdf5d2ac9c161bf1562ecce30cb3e2efd21d
    SOURCE_SUBDIR  tmxlite
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(SDL3 tmxlite)

# tmxlite only defines TMXLITE_STATIC for its own Debug/Release flags and does
# not export it, so consumers would try to dllimport the symbols.
target_compile_definitions(tmxlite PUBLIC TMXLITE_STATIC)

# --- D3DX9 + DxErr shim (Windows, temporary) --------------------------------
# D3DX9 is not part of the Windows SDK. Microsoft publishes the June 2010
# headers/libs/redist as a NuGet package, which avoids needing the legacy
# DirectX SDK installer (and its hard-coded C:\Program Files (x86) path).
# Removed in phase 4 when the D3D9 renderer is replaced.
if(WIN32)
    FetchContent_Declare(d3dx9_pkg
        URL            https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/9.29.952.8/microsoft.dxsdk.d3dx.9.29.952.8.nupkg
        URL_HASH       SHA256=ead0906ae8a26c18a7525da7490127a2110f7c58f18293738283e30e97c6ea4b
        DOWNLOAD_NAME  microsoft.dxsdk.d3dx.9.29.952.8.zip
    )
    FetchContent_MakeAvailable(d3dx9_pkg)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_d3dx9_arch x64)
    else()
        set(_d3dx9_arch x86)
    endif()
    set(_d3dx9_native "${d3dx9_pkg_SOURCE_DIR}/build/native")

    add_library(sdlgf_d3dx9 INTERFACE)
    target_include_directories(sdlgf_d3dx9 INTERFACE
        "${_d3dx9_native}/include"
        "${CMAKE_SOURCE_DIR}/cmake/shims")  # DxErr.h (not in the NuGet package)
    # The release import lib is used for every configuration: the debug one
    # needs the DirectX debug runtime, which is not installed by default.
    target_link_libraries(sdlgf_d3dx9 INTERFACE
        "${_d3dx9_native}/release/lib/${_d3dx9_arch}/d3dx9.lib")
    set(SDLGF_D3DX9_RUNTIME_DLL "${_d3dx9_native}/release/bin/${_d3dx9_arch}/D3DX9_43.dll")
endif()
