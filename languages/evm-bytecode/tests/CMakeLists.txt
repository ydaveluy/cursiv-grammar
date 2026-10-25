# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean, and this
# port's own coverage document (tests/coverage.evm) exercises every mnemonic.
add_executable(evmbytecode_grammar_test grammar_test.cpp)
target_link_libraries(evmbytecode_grammar_test PRIVATE evmbytecodeCore cursiv::test cursiv::lsp)
target_compile_definitions(evmbytecode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME evmbytecode_grammar_test COMMAND evmbytecode_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET evmbytecode_cli)
  add_test(NAME evmbytecode_cli_test COMMAND evmbytecode_cli
           ${PROJECT_SOURCE_DIR}/example/0xcDA72070E455bb31C7690a170224Ce43623d0B6f.evm)
endif()
