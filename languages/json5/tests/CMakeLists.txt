# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Doc.
add_executable(json5_grammar_test grammar_test.cpp)
target_link_libraries(json5_grammar_test PRIVATE json5Core cursiv::test cursiv::lsp)
target_compile_definitions(json5_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME json5_grammar_test COMMAND json5_grammar_test)

# ...and the command line reads one example.
if(TARGET json5_cli)
  add_test(NAME json5_cli_test COMMAND json5_cli
           ${PROJECT_SOURCE_DIR}/example/example2.json5)
endif()
