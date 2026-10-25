# The upstream corpus ships no examples/ at all, so every document lives
# here rather than under example/. Each parses clean into a Document.
add_executable(bcl_grammar_test grammar_test.cpp)
target_link_libraries(bcl_grammar_test PRIVATE bclCore)
target_compile_definitions(bcl_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bcl_grammar_test COMMAND bcl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME bcl_cli_test COMMAND bcl_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/program.bcl)
