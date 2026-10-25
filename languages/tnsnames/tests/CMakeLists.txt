add_executable(tnsnames_grammar_test grammar_test.cpp)
target_link_libraries(tnsnames_grammar_test PRIVATE tnsnamesCore)
target_compile_definitions(tnsnames_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tnsnames_grammar_test COMMAND tnsnames_grammar_test)

# ...and the command line reads the same corpus file.
add_test(NAME tnsnames_cli_test COMMAND tnsnames_cli
         ${PROJECT_SOURCE_DIR}/example/tnsnames.test.ora)
