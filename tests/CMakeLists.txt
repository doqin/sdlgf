# --- Platform layer (time, UTF conversion, executable path) ---------------------------
add_executable(sdlgf_platform_test
    platform_test.cpp
    "${CMAKE_SOURCE_DIR}/core/Platform.cpp"
)
target_include_directories(sdlgf_platform_test PRIVATE "${CMAKE_SOURCE_DIR}/core")
target_link_libraries(sdlgf_platform_test PRIVATE SDL3::SDL3)   # Platform uses SDL's message box
add_test(NAME platform COMMAND sdlgf_platform_test)

# Compile-only: verifies the non-Windows type stand-ins have Win32-compatible layout.
add_library(sdlgf_portable_types_check OBJECT portable_types_check.cpp)
target_include_directories(sdlgf_portable_types_check PRIVATE "${CMAKE_SOURCE_DIR}/core")
add_dependencies(sdlgf_platform_test sdlgf_portable_types_check)

# --- Key translation (DIK <-> SDL scancode) -----------------------------------------
add_executable(sdlgf_keys_test
    keys_test.cpp
    "${CMAKE_SOURCE_DIR}/core/Keys.cpp"
)
target_include_directories(sdlgf_keys_test PRIVATE "${CMAKE_SOURCE_DIR}/core")
target_link_libraries(sdlgf_keys_test PRIVATE SDL3::SDL3)
add_test(NAME keys COMMAND sdlgf_keys_test)

# --- Application window + InputManager, headless (SDL "dummy" video driver) ---------
add_executable(sdlgf_input_test input_test.cpp)
target_link_libraries(sdlgf_input_test PRIVATE sdlgf::core)
add_test(NAME input COMMAND sdlgf_input_test)

# --- Static checks --------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    # Include/asset path case + backslashes
    add_test(NAME portability_lint
             COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_portability.py")
    # Every DIK -> SDL scancode row agrees with SDL's own Windows scancode table
    set(_sdl_win_scancodes "${sdl3_SOURCE_DIR}/src/events/scancodes_windows.h")
    if(EXISTS "${_sdl_win_scancodes}")
        add_test(NAME key_table
                 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_key_table.py" "${_sdl_win_scancodes}")
    endif()
endif()

if(WIN32)
    # Keys.h must agree with the real <dinput.h>: a differing value is a macro
    # redefinition (C4005), which this target turns into an error.
    add_library(sdlgf_dik_values_check OBJECT dik_values_check.cpp)
    target_include_directories(sdlgf_dik_values_check PRIVATE "${CMAKE_SOURCE_DIR}/core")
    target_compile_options(sdlgf_dik_values_check PRIVATE /we4005)
    add_dependencies(sdlgf_keys_test sdlgf_dik_values_check)
endif()
