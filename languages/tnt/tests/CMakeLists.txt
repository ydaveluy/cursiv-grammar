# The grammar itself: every example and test document parses clean into an
# Equation, and the operator ladder's precedence holds.
add_executable(tnt_grammar_test grammar_test.cpp)
target_link_libraries(tnt_grammar_test PRIVATE tntCore cursiv::test)
target_compile_definitions(tnt_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tnt_grammar_test COMMAND tnt_grammar_test)

# ...and the command line reads one example.
add_test(NAME tnt_cli_test COMMAND tnt_cli
         ${PROJECT_SOURCE_DIR}/example/commutative.tnt)
