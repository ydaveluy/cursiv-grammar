# The lane of this language: one binary, the example directory is where its
# corpus is read from.
add_executable(wktcrsv1_grammar_test grammar_test.cpp)
target_link_libraries(wktcrsv1_grammar_test PRIVATE wktcrsv1Core cursiv::test)
target_compile_definitions(wktcrsv1_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME wktcrsv1_grammar_test COMMAND wktcrsv1_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME wktcrsv1_cli_test COMMAND wktcrsv1_cli
         ${PROJECT_SOURCE_DIR}/example/example1.wktcrsv1)
