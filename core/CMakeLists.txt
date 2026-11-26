include(sources.cmake)

add_library(sdlgf_core STATIC ${SDLGF_CORE_SOURCES})
add_library(sdlgf::core ALIAS sdlgf_core)

target_include_directories(sdlgf_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/audio"
    "${CMAKE_CURRENT_SOURCE_DIR}/graphics"
    "${CMAKE_CURRENT_SOURCE_DIR}/input"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform"
    "${CMAKE_CURRENT_SOURCE_DIR}/scene"
    "${CMAKE_CURRENT_SOURCE_DIR}/util"
    "${CMAKE_SOURCE_DIR}/includes"
)

# Shared precompiled header (also used by demo); sources include "pch.h" first.
target_precompile_headers(sdlgf_core PRIVATE "${CMAKE_SOURCE_DIR}/includes/pch.h")

target_compile_definitions(sdlgf_core PUBLIC UNICODE _UNICODE NOMINMAX)

# MSVC defines _DEBUG itself for /MDd; other compilers don't, and the demo's debug overlay
# (FPS title, ImGui battle window) is guarded by #ifdef _DEBUG.
if(NOT MSVC)
    target_compile_definitions(sdlgf_core PUBLIC $<$<CONFIG:Debug>:_DEBUG>)
endif()

find_package(Threads REQUIRED)

target_link_libraries(sdlgf_core PUBLIC
    Threads::Threads
    tmxlite
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
)

if(MSVC)
    target_compile_options(sdlgf_core PUBLIC /permissive- /W3 /MP)
endif()
