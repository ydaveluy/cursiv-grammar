# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(stacktrace_grammar_test grammar_test.cpp)
target_link_libraries(stacktrace_grammar_test PRIVATE stacktraceCore)
target_compile_definitions(stacktrace_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME stacktrace_grammar_test COMMAND stacktrace_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME stacktrace_cli_test COMMAND stacktrace_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/simple.stacktrace)
