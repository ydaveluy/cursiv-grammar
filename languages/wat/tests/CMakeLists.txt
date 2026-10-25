add_executable(wat_grammar_test grammar_test.cpp)
target_link_libraries(wat_grammar_test PRIVATE watCore)
target_compile_definitions(wat_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME wat_grammar_test COMMAND wat_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME wat_cli_test COMMAND wat_cli
         ${PROJECT_SOURCE_DIR}/example/shared1.wat)
