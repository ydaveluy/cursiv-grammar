# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(angelscript_grammar_test grammar_test.cpp)
target_link_libraries(angelscript_grammar_test PRIVATE angelscriptCore)
target_compile_definitions(angelscript_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME angelscript_grammar_test COMMAND angelscript_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME angelscript_cli_test COMMAND angelscript_cli
         ${PROJECT_SOURCE_DIR}/example/example1.as)
