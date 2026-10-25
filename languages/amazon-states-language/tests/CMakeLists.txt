# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus example parses clean, plus a document of
# this port's own for fields the corpus never reaches.
add_executable(amazonstateslanguage_grammar_test grammar_test.cpp)
target_link_libraries(amazonstateslanguage_grammar_test PRIVATE amazonstateslanguageCore cursiv::test cursiv::lsp)
target_compile_definitions(amazonstateslanguage_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME amazonstateslanguage_grammar_test COMMAND amazonstateslanguage_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET amazonstateslanguage_cli)
  add_test(NAME amazonstateslanguage_cli_test COMMAND amazonstateslanguage_cli
           ${PROJECT_SOURCE_DIR}/example/choice_state.asl)
endif()
