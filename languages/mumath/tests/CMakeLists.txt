# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ and tests/ parses, and
# the tree holds what it declares -- operator precedence included.
add_executable(mumath_grammar_test grammar_test.cpp)
target_link_libraries(mumath_grammar_test PRIVATE mumathCore)
target_compile_definitions(mumath_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME mumath_grammar_test COMMAND mumath_grammar_test)

# ...and the command line reads one of them.
if(TARGET mumath_cli)
  add_test(NAME mumath_cli_test COMMAND mumath_cli
           ${PROJECT_SOURCE_DIR}/example/example1.mu)
endif()
