include(sources.cmake)

add_library(sdlgf_core STATIC ${SDLGF_CORE_SOURCES})
add_library(sdlgf::core ALIAS sdlgf_core)

target_include_directories(sdlgf_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/includes"
)

# Shared precompiled header (also used by demo); sources include "pch.h" first.
target_precompile_headers(sdlgf_core PRIVATE "${CMAKE_SOURCE_DIR}/includes/pch.h")

target_compile_definitions(sdlgf_core PUBLIC UNICODE _UNICODE)

target_link_libraries(sdlgf_core PUBLIC
    tmxlite
    sdlgf_d3dx9
    d3d9 dinput8 dxguid xaudio2 winmm
    legacy_stdio_definitions
)

if(MSVC)
    target_compile_options(sdlgf_core PUBLIC /permissive- /W3 /MP)
endif()
