# The grammar itself: every document under example/ (upstream) and tests/
# (constructs the corpus never reaches) parses clean, and the tree holds
# what it declares.
add_executable(action_grammar_test grammar_test.cpp)
target_link_libraries(action_grammar_test PRIVATE actionCore)
target_compile_definitions(action_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME action_grammar_test COMMAND action_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME action_cli_test COMMAND action_cli
         ${PROJECT_SOURCE_DIR}/example/primes.act)
