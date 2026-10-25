# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(d2_grammar_test grammar_test.cpp)
target_link_libraries(d2_grammar_test PRIVATE d2Core)
target_compile_definitions(d2_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME d2_grammar_test COMMAND d2_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET d2_cli)
  add_test(NAME d2_cli_test COMMAND d2_cli
           ${PROJECT_SOURCE_DIR}/example/example2.d2)
endif()
