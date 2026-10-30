# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document in example/ parses clean, and a
# few field values read back what the AST holds.
add_executable(lucene_grammar_test grammar_test.cpp)
target_link_libraries(lucene_grammar_test PRIVATE luceneCore cursiv::test)
target_compile_definitions(lucene_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lucene_grammar_test COMMAND lucene_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET lucene_cli)
  add_test(NAME lucene_cli_test COMMAND lucene_cli
           ${PROJECT_SOURCE_DIR}/example/boolean-1.lucene)
endif()
