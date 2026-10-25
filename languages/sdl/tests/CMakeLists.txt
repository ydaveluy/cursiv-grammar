# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, and one document's AST
# holds what it declares.
add_executable(sdl_grammar_test grammar_test.cpp)
target_link_libraries(sdl_grammar_test PRIVATE sdlCore)
target_compile_definitions(sdl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sdl_grammar_test COMMAND sdl_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET sdl_cli)
  add_test(NAME sdl_cli_test COMMAND sdl_cli
           ${PROJECT_SOURCE_DIR}/example/test1.sdl)
endif()
