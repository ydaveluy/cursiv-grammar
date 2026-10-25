# The grammar itself: every example parses clean into an Itinerary.
add_executable(itn_grammar_test grammar_test.cpp)
target_link_libraries(itn_grammar_test PRIVATE itnCore cursiv::test)
target_compile_definitions(itn_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME itn_grammar_test COMMAND itn_grammar_test)

# ...and the command line reads one example.
add_test(NAME itn_cli_test COMMAND itn_cli
         ${PROJECT_SOURCE_DIR}/example/example1.itn)
