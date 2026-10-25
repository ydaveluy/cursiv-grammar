# The upstream corpus ships no examples/ at all, so every document lives
# here rather than under example/. Each tree holds what it declares.
add_executable(pmmn_grammar_test grammar_test.cpp)
target_link_libraries(pmmn_grammar_test PRIVATE pmmnCore)
target_compile_definitions(pmmn_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pmmn_grammar_test COMMAND pmmn_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME pmmn_cli_test COMMAND pmmn_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/add.pmmn)
