# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(eiffel_grammar_test grammar_test.cpp)
target_link_libraries(eiffel_grammar_test PRIVATE eiffelCore)
target_compile_definitions(eiffel_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME eiffel_grammar_test COMMAND eiffel_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME eiffel_cli_test COMMAND eiffel_cli
         ${PROJECT_SOURCE_DIR}/example/100_doors/application.e)
