# The grammar itself: every example parses clean into a Map.
add_executable(quakemap_grammar_test grammar_test.cpp)
target_link_libraries(quakemap_grammar_test PRIVATE quakemapCore)
target_compile_definitions(quakemap_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME quakemap_grammar_test COMMAND quakemap_grammar_test)

# ...and the command line reads one example.
add_test(NAME quakemap_cli_test COMMAND quakemap_cli
         ${PROJECT_SOURCE_DIR}/example/example1.map)
