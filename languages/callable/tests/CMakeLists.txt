# The grammar itself: every example parses, and each tree holds a program.
add_executable(callable_grammar_test grammar_test.cpp)
target_link_libraries(callable_grammar_test PRIVATE callableCore)
target_compile_definitions(callable_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME callable_grammar_test COMMAND callable_grammar_test)

# ...and the command line reads one of them.
add_test(NAME callable_cli_test COMMAND callable_cli
         ${PROJECT_SOURCE_DIR}/example/example3.callable)
