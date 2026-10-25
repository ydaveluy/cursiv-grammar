# The grammar itself: every document under example/ parses clean.
add_executable(jam_grammar_test grammar_test.cpp)
target_link_libraries(jam_grammar_test PRIVATE jamCore)
target_compile_definitions(jam_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME jam_grammar_test COMMAND jam_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME jam_cli_test COMMAND jam_cli
         ${PROJECT_SOURCE_DIR}/example/example1.jam)
