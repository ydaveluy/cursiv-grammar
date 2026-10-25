# The grammar itself: every example parses clean into a Doc.
add_executable(csv_grammar_test grammar_test.cpp)
target_link_libraries(csv_grammar_test PRIVATE csvCore cursiv::test)
target_compile_definitions(csv_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME csv_grammar_test COMMAND csv_grammar_test)

# ...and the command line reads one example.
add_test(NAME csv_cli_test COMMAND csv_cli
         ${PROJECT_SOURCE_DIR}/example/example1.csv)
