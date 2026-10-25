# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses clean, and the tree holds
# what a few of them declare.
add_executable(pike_grammar_test grammar_test.cpp)
target_link_libraries(pike_grammar_test PRIVATE pikeCore)
target_compile_definitions(pike_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pike_grammar_test COMMAND pike_grammar_test)

# ...and the command line reads one corpus sample.
if(TARGET pike_cli)
  add_test(NAME pike_cli_test COMMAND pike_cli
           ${PROJECT_SOURCE_DIR}/example/example1.pike)
endif()

add_executable(pike_integer_test integer_test.cpp)
target_link_libraries(pike_integer_test PRIVATE pikeCore cursiv::test)
target_compile_definitions(pike_integer_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pike_integer_test COMMAND pike_integer_test)
