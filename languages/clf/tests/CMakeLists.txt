# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus file parses, and the fields it writes hold.
add_executable(clf_grammar_test grammar_test.cpp)
target_link_libraries(clf_grammar_test PRIVATE clfCore)
target_compile_definitions(clf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME clf_grammar_test COMMAND clf_grammar_test)

# ...and the command line reads one of the same files.
if(TARGET clf_cli)
  add_test(NAME clf_cli_test COMMAND clf_cli
           ${PROJECT_SOURCE_DIR}/example/common1.clf)
endif()
