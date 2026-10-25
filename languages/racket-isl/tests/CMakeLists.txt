# The grammar itself: every file of example/ parses clean, and the tree
# holds what each sample declares.
add_executable(racketisl_grammar_test grammar_test.cpp)
target_link_libraries(racketisl_grammar_test PRIVATE racketislCore)
target_compile_definitions(racketisl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME racketisl_grammar_test COMMAND racketisl_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME racketisl_cli_test COMMAND racketisl_cli
         ${PROJECT_SOURCE_DIR}/example/list-contains.racketisl)
