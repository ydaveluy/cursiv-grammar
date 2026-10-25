# The grammar itself: every example parses, and the tree holds a Document.
add_executable(stellaris_grammar_test grammar_test.cpp)
target_link_libraries(stellaris_grammar_test PRIVATE stellarisCore)
target_compile_definitions(stellaris_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME stellaris_grammar_test COMMAND stellaris_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME stellaris_cli_test COMMAND stellaris_cli
         ${PROJECT_SOURCE_DIR}/example/ongoing_events.stellaris)
