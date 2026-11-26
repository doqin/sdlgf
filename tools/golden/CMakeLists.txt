add_executable(sdlgf_golden_gen golden_gen.cpp)
target_include_directories(sdlgf_golden_gen PRIVATE "${CMAKE_SOURCE_DIR}/tests/golden")
target_link_libraries(sdlgf_golden_gen PRIVATE sdlgf::core)
target_compile_definitions(sdlgf_golden_gen PRIVATE
    SDLGF_GOLDEN_ASSET_DIR="${CMAKE_SOURCE_DIR}/demo")
