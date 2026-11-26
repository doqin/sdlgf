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

# --- SDL_ttf ----------------------------------------------------------------
# Text rendering. Only FreeType is needed (vendored as a git submodule of SDL_ttf);
# HarfBuzz (complex-script shaping) and PlutoSVG (colour emoji) are switched off.
# Everything is linked statically (SDL_ttf otherwise looks for the shared SDL3 target).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-3.2.2
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES external/freetype
    EXCLUDE_FROM_ALL
)

# --- SDL_image --------------------------------------------------------------
# Image loading. SDL itself only reads PNG and BMP; the game also has JPEG tilesets (XP.jpg).
# Built with its bundled stb_image decoders only, the same on every platform: the native
# decoders (WIC on Windows, ImageIO on macOS) and the formats that need vendored libraries
# (AVIF, JPEG XL, TIFF, WebP, SVG) and the rarely used ones are switched off: only PNG, JPEG,
# GIF and BMP remain.
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_WIC OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_ANI OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_SAVE OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.4.8
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""
    EXCLUDE_FROM_ALL
)

# --- SDL_mixer --------------------------------------------------------------
# Used only to decode compressed audio (Ogg Vorbis) into memory through MIX_AudioDecoder; the
# game keeps its own software mixer (core/AudioMixer) for playback, stems and gapless intros.
# Only the built-in WAVE decoder and stb_vorbis are enabled: nothing vendored, no extra
# libraries, so no git submodules are fetched and no shared libraries are loaded at runtime.
set(SDLMIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_AIFF OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VOC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_AU OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_STB ON CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "" FORCE)
set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG        release-3.2.4
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(SDL3_image SDL3_ttf SDL3_mixer tmxlite)

# tmxlite only defines TMXLITE_STATIC for its own Debug/Release flags and does
# not export it, so consumers would try to dllimport the symbols.
target_compile_definitions(tmxlite PUBLIC TMXLITE_STATIC)

# --- D3DX9 (Windows, optional, tests only) ---------------------------------
# The game no longer uses Direct3D or D3DX. This is only for the math test, which can compare
# the replacement math library against Microsoft's original implementation. Microsoft publishes
# the June 2010 headers/libs/redist as a NuGet package, which avoids the legacy DirectX SDK
# installer. Off by default: -DSDLGF_D3DX_CROSSCHECK=ON.
if(WIN32 AND SDLGF_D3DX_CROSSCHECK)
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
    target_include_directories(sdlgf_d3dx9 INTERFACE "${_d3dx9_native}/include")
    target_link_libraries(sdlgf_d3dx9 INTERFACE
        "${_d3dx9_native}/release/lib/${_d3dx9_arch}/d3dx9.lib")
    set(SDLGF_D3DX9_RUNTIME_DLL "${_d3dx9_native}/release/bin/${_d3dx9_arch}/D3DX9_43.dll")
endif()
