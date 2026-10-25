# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(dot_grammar_test grammar_test.cpp)
target_link_libraries(dot_grammar_test PRIVATE dotCore)
target_compile_definitions(dot_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME dot_grammar_test COMMAND dot_grammar_test)

add_executable(dot_native_regressions native_regressions.cpp)
target_link_libraries(dot_native_regressions PRIVATE dotCore cursiv::test)
add_test(NAME dot_native_regressions COMMAND dot_native_regressions)

# ...and the command line reads one of the same samples.
add_test(NAME dot_cli_test COMMAND dot_cli
         ${PROJECT_SOURCE_DIR}/example/crazy.dot)
