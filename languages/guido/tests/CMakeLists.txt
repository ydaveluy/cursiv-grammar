# The grammar itself: every example parses clean into a Prog.
add_executable(guido_grammar_test grammar_test.cpp)
target_link_libraries(guido_grammar_test PRIVATE guidoCore cursiv::test)
target_compile_definitions(guido_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME guido_grammar_test COMMAND guido_grammar_test)

# ...and the command line reads one example.
add_test(NAME guido_cli_test COMMAND guido_cli
         ${PROJECT_SOURCE_DIR}/example/example1.gmn)
