# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(srt_grammar_test grammar_test.cpp)
target_link_libraries(srt_grammar_test PRIVATE srtCore cursiv::test cursiv::lsp)
target_compile_definitions(srt_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME srt_grammar_test COMMAND srt_grammar_test)

# ...and the command line reads one of them.
add_test(NAME srt_cli_test COMMAND srt_cli
         ${PROJECT_SOURCE_DIR}/example/srt_example.srt)
