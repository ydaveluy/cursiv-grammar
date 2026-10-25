# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses clean, a handful are read
# back field by field, and a coverage document of our own exercises what the
# corpus never does.
add_executable(smiles_grammar_test grammar_test.cpp)
target_link_libraries(smiles_grammar_test PRIVATE smilesCore cursiv::test)
target_compile_definitions(smiles_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME smiles_grammar_test COMMAND smiles_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET smiles_cli)
  add_test(NAME smiles_cli_test COMMAND smiles_cli
           ${PROJECT_SOURCE_DIR}/example/methane.smi)
endif()
