# The grammar itself: every example parses clean into a Document.
add_executable(gedcom_grammar_test grammar_test.cpp)
target_link_libraries(gedcom_grammar_test PRIVATE gedcomCore cursiv::test)
target_compile_definitions(gedcom_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gedcom_grammar_test COMMAND gedcom_grammar_test)

# ...and the command line reads one of them.
add_test(NAME gedcom_cli_test COMMAND gedcom_cli
         ${PROJECT_SOURCE_DIR}/example/example1.ged)
