# The grammar itself: every file of example/ parses, and each builds a Sexpr.
add_executable(sexpression_grammar_test grammar_test.cpp)
target_link_libraries(sexpression_grammar_test PRIVATE sexpressionCore)
target_compile_definitions(sexpression_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sexpression_grammar_test COMMAND sexpression_grammar_test)

# ...and the command line reads one of them.
add_test(NAME sexpression_cli_test COMMAND sexpression_cli
         ${PROJECT_SOURCE_DIR}/example/example1.sexpression)
