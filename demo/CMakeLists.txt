include(sources.cmake)

add_executable(sdlgf_demo WIN32 ${SDLGF_DEMO_SOURCES})
set_target_properties(sdlgf_demo PROPERTIES OUTPUT_NAME demo)

target_include_directories(sdlgf_demo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/cards/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/cards/targeting"
    "${CMAKE_CURRENT_SOURCE_DIR}/cards/registry"
    "${CMAKE_CURRENT_SOURCE_DIR}/cards/defs"
    "${CMAKE_SOURCE_DIR}/core/Extras"
)

target_link_libraries(sdlgf_demo PRIVATE sdlgf::core)

if(SDLGF_TESTING)
    target_compile_definitions(sdlgf_demo PRIVATE TESTING)
endif()

# The vendored Dear ImGui sources were compiled without the precompiled header.
target_precompile_headers(sdlgf_demo PRIVATE "${CMAKE_SOURCE_DIR}/includes/pch.h")
set_source_files_properties(
    backends/imgui_impl_sdlrenderer3.cpp
    backends/imgui_impl_sdl3.cpp
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    misc/cpp/imgui_stdlib.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# The game loads "assets/..." relative to the executable.
if(SDLGF_COPY_ASSETS)
    add_custom_command(TARGET sdlgf_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:sdlgf_demo>/assets"
        VERBATIM
    )
endif()
