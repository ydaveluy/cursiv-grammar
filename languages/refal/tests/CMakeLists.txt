# The grammar itself: what this language takes, and what it refuses.
add_executable(refal_grammar_test grammar_test.cpp)
target_link_libraries(refal_grammar_test PRIVATE refalCore)
target_compile_definitions(refal_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME refal_grammar_test COMMAND refal_grammar_test)

# ...and the command line reads one document of it.
add_test(NAME refal_cli_test COMMAND refal_cli
         ${PROJECT_SOURCE_DIR}/example/example1.ref)
