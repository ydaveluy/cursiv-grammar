# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(telephone_grammar_test grammar_test.cpp)
target_link_libraries(telephone_grammar_test PRIVATE telephoneCore)
target_compile_definitions(telephone_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME telephone_grammar_test COMMAND telephone_grammar_test)

# ...and the command line reads one of them.
add_test(NAME telephone_cli_test COMMAND telephone_cli
         ${PROJECT_SOURCE_DIR}/example/example1.tel)
