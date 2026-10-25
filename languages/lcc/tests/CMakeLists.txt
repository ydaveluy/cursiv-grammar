# The grammar itself: every example parses, and each tree holds a call number.
add_executable(lcc_grammar_test grammar_test.cpp)
target_link_libraries(lcc_grammar_test PRIVATE lccCore cursiv::test)
target_compile_definitions(lcc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lcc_grammar_test COMMAND lcc_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME lcc_cli_test COMMAND lcc_cli
         ${PROJECT_SOURCE_DIR}/example/eg1.lcc)
