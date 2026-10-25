# The grammar itself: every document under example/ parses clean and holds
# what it declares.
add_executable(solidity_grammar_test grammar_test.cpp)
target_link_libraries(solidity_grammar_test PRIVATE solidityCore)
target_compile_definitions(solidity_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME solidity_grammar_test COMMAND solidity_grammar_test)

# ...and the command line reads one sample.
add_test(NAME solidity_cli_test COMMAND solidity_cli
         ${PROJECT_SOURCE_DIR}/example/small_test.sol)
