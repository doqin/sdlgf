add_executable(sdlgf_platform_test
    platform_test.cpp
    "${CMAKE_SOURCE_DIR}/core/DX9GFPlatform.cpp"
)
target_include_directories(sdlgf_platform_test PRIVATE "${CMAKE_SOURCE_DIR}/core")
add_test(NAME platform COMMAND sdlgf_platform_test)

# Compile-only: verifies the non-Windows type stand-ins have Win32-compatible layout.
add_library(sdlgf_portable_types_check OBJECT portable_types_check.cpp)
target_include_directories(sdlgf_portable_types_check PRIVATE "${CMAKE_SOURCE_DIR}/core")
add_dependencies(sdlgf_platform_test sdlgf_portable_types_check)

# Portability lint: include/asset path case + backslashes. Needs Python 3.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME portability_lint
             COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_portability.py")
endif()
