# The grammar itself: every example parses clean, and each tree is a Document.
add_executable(stl_grammar_test grammar_test.cpp)
target_link_libraries(stl_grammar_test PRIVATE stlCore cursiv::test)
target_compile_definitions(stl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME stl_grammar_test COMMAND stl_grammar_test)

# ...and the command line reads one of them.
add_test(NAME stl_cli_test COMMAND stl_cli
         ${PROJECT_SOURCE_DIR}/tests/triangle.stl)
