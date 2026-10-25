# The grammar itself: every document in example/ parses, clean.
add_executable(alloy_grammar_test grammar_test.cpp)
target_link_libraries(alloy_grammar_test PRIVATE alloyCore)
target_compile_definitions(alloy_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME alloy_grammar_test COMMAND alloy_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME alloy_cli_test COMMAND alloy_cli
         ${PROJECT_SOURCE_DIR}/example/fs.als)
