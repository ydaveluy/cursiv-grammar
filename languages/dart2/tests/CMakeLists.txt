# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(dart2_grammar_test grammar_test.cpp)
target_link_libraries(dart2_grammar_test PRIVATE dart2Core)
target_compile_definitions(dart2_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME dart2_grammar_test COMMAND dart2_grammar_test)

# ...and the command line reads one of them.
if(TARGET dart2_cli)
  add_test(NAME dart2_cli_test COMMAND dart2_cli
           ${PROJECT_SOURCE_DIR}/example/hw.dart)
endif()
