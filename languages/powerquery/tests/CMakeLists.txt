# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the corpus, our own coverage document, and a spot
# check of field values read back out of the AST.
add_executable(powerquery_grammar_test grammar_test.cpp)
target_link_libraries(powerquery_grammar_test PRIVATE powerqueryCore)
target_compile_definitions(powerquery_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME powerquery_grammar_test COMMAND powerquery_grammar_test)

# ...and the command line reads the same corpus document.
if(TARGET powerquery_cli)
  add_test(NAME powerquery_cli_test COMMAND powerquery_cli
           ${PROJECT_SOURCE_DIR}/example/example.pq)
endif()
