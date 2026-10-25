# The grammar itself: every document under tests/ parses, clean.
add_executable(pgn_grammar_test grammar_test.cpp)
target_link_libraries(pgn_grammar_test PRIVATE pgnCore)
target_compile_definitions(pgn_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pgn_grammar_test COMMAND pgn_grammar_test)

# ...and the command line reads one of them.
add_test(NAME pgn_cli_test COMMAND pgn_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/game.pgn)
