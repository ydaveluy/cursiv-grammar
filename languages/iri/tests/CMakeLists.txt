# The grammar itself: every document under example/ and tests/ parses, and
# the tree holds what it declares.
add_executable(iri_grammar_test grammar_test.cpp)
target_link_libraries(iri_grammar_test PRIVATE iriCore)
target_compile_definitions(iri_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME iri_grammar_test COMMAND iri_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME iri_cli_test COMMAND iri_cli
         ${PROJECT_SOURCE_DIR}/example/example1.iri)
