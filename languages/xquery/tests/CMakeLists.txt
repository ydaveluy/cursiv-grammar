# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, features.xq covers what
# the corpus doesn't, and module.xq is read back field by field.
add_executable(xquery_grammar_test grammar_test.cpp)
target_link_libraries(xquery_grammar_test PRIVATE xqueryCore)
target_compile_definitions(xquery_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.xq")
add_test(NAME xquery_grammar_test COMMAND xquery_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET xquery_cli)
  add_test(NAME xquery_cli_test COMMAND xquery_cli
           ${PROJECT_SOURCE_DIR}/example/module.xq)
endif()
