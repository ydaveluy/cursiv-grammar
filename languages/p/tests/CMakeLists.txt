# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(p_grammar_test grammar_test.cpp)
target_link_libraries(p_grammar_test PRIVATE pCore)
target_compile_definitions(p_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME p_grammar_test COMMAND p_grammar_test)

# ...and the command line reads one of them.
add_test(NAME p_cli_test COMMAND p_cli
         ${PROJECT_SOURCE_DIR}/example/example1.p)
