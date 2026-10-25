# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(parkingsign_grammar_test grammar_test.cpp)
target_link_libraries(parkingsign_grammar_test PRIVATE parkingsignCore)
target_compile_definitions(parkingsign_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME parkingsign_grammar_test COMMAND parkingsign_grammar_test)

# ...and the command line reads the same sample.
add_test(NAME parkingsign_cli_test COMMAND parkingsign_cli
         ${PROJECT_SOURCE_DIR}/example/example1.parkingsign)
