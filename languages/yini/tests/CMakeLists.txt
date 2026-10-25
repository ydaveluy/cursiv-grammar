# The grammar itself: every example parses, and a sample of field values
# comes back through the converters that built them.
add_executable(yini_grammar_test grammar_test.cpp)
target_link_libraries(yini_grammar_test PRIVATE yiniCore cursiv::test)
target_compile_definitions(yini_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CURSIV_INVALID_KEY="${CMAKE_CURRENT_SOURCE_DIR}/invalid-key.yini")
add_test(NAME yini_grammar_test COMMAND yini_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME yini_cli_test COMMAND yini_cli
         ${PROJECT_SOURCE_DIR}/example/Short-1.yini)
