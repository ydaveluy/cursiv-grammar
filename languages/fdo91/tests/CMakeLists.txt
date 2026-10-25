# The grammar itself: every document in example/ parses, and its root is a Doc.
add_executable(fdo91_grammar_test grammar_test.cpp)
target_link_libraries(fdo91_grammar_test PRIVATE fdo91Core)
target_compile_definitions(fdo91_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fdo91_grammar_test COMMAND fdo91_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME fdo91_cli_test COMMAND fdo91_cli
         ${PROJECT_SOURCE_DIR}/example/example12.fdo91)
