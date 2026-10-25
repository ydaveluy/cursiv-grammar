# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(inf_grammar_test grammar_test.cpp)
target_link_libraries(inf_grammar_test PRIVATE infCore)
target_compile_definitions(inf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME inf_grammar_test COMMAND inf_grammar_test)

# ...and the command line reads one example.
if(TARGET inf_cli)
  add_test(NAME inf_cli_test COMMAND inf_cli
           ${PROJECT_SOURCE_DIR}/example/example3.inf)
endif()
