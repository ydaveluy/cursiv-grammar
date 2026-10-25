# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(b_grammar_test grammar_test.cpp)
target_link_libraries(b_grammar_test PRIVATE bCore cursiv::test)
target_compile_definitions(b_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME b_grammar_test COMMAND b_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET b_cli)
  add_test(NAME b_cli_test COMMAND b_cli
           ${PROJECT_SOURCE_DIR}/example/example1.b)
endif()

set_tests_properties(b_grammar_test PROPERTIES TIMEOUT 30)
