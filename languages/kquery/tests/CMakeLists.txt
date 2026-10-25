# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the corpus parses, and the tree holds what it declares.
add_executable(kquery_grammar_test grammar_test.cpp)
target_link_libraries(kquery_grammar_test PRIVATE kqueryCore)
target_compile_definitions(kquery_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME kquery_grammar_test COMMAND kquery_grammar_test)

# ...and the command line reads one corpus file.
if(TARGET kquery_cli)
  add_test(NAME kquery_cli_test COMMAND kquery_cli
           ${PROJECT_SOURCE_DIR}/example/test1.kquery)
endif()
