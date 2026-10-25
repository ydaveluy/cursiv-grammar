# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, rooted at Program.
add_executable(ttm_grammar_test grammar_test.cpp)
target_link_libraries(ttm_grammar_test PRIVATE ttmCore)
target_compile_definitions(ttm_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ttm_grammar_test COMMAND ttm_grammar_test)

# ...and the command line reads one of them.
if(TARGET ttm_cli)
  add_test(NAME ttm_cli_test COMMAND ttm_cli
           ${PROJECT_SOURCE_DIR}/example/example1.ttm)
endif()
