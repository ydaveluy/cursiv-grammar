# The grammar itself: every corpus example parses, and the tree holds what
# the reference's own tree agrees on.
add_executable(graphemes_grammar_test grammar_test.cpp)
target_link_libraries(graphemes_grammar_test PRIVATE graphemesCore)
target_compile_definitions(graphemes_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME graphemes_grammar_test COMMAND graphemes_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME graphemes_cli_test COMMAND graphemes_cli
         ${PROJECT_SOURCE_DIR}/example/udhr.graphemes)
