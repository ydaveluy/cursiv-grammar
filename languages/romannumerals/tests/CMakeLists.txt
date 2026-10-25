# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(romannumerals_grammar_test grammar_test.cpp)
target_link_libraries(romannumerals_grammar_test PRIVATE romannumeralsCore)
target_compile_definitions(romannumerals_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME romannumerals_grammar_test COMMAND romannumerals_grammar_test)

# ...and the command line reads one example.
add_test(NAME romannumerals_cli_test COMMAND romannumerals_cli
         ${PROJECT_SOURCE_DIR}/example/MCMLXXII.romannumerals)
