# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(lpc_grammar_test grammar_test.cpp)
target_link_libraries(lpc_grammar_test PRIVATE lpcCore cursiv::test cursiv::lsp)
target_compile_definitions(lpc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lpc_grammar_test COMMAND lpc_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET lpc_cli)
  add_test(NAME lpc_cli_test COMMAND lpc_cli
           ${PROJECT_SOURCE_DIR}/example/example1.lpc)
endif()
