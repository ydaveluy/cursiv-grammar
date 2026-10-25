# The grammar itself: every document under example/ parses clean.
add_executable(fen_grammar_test grammar_test.cpp)
target_link_libraries(fen_grammar_test PRIVATE fenCore cursiv::test cursiv::lsp)
target_compile_definitions(fen_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fen_grammar_test COMMAND fen_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME fen_cli_test COMMAND fen_cli
         ${PROJECT_SOURCE_DIR}/example/example1.fen)
