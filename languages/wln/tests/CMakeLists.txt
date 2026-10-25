# The grammar itself: every example parses clean, with nothing left over.
add_executable(wln_grammar_test grammar_test.cpp)
target_link_libraries(wln_grammar_test PRIVATE wlnCore cursiv::test)
target_compile_definitions(wln_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME wln_grammar_test COMMAND wln_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME wln_cli_test COMMAND wln_cli
         ${PROJECT_SOURCE_DIR}/example/phenylalanine.wln)
