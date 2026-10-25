# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(bibcode_grammar_test grammar_test.cpp)
target_link_libraries(bibcode_grammar_test PRIVATE bibcodeCore cursiv::test)
target_compile_definitions(bibcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME bibcode_grammar_test COMMAND bibcode_grammar_test)

# ...and the command line reads one of them.
add_test(NAME bibcode_cli_test COMMAND bibcode_cli
         ${PROJECT_SOURCE_DIR}/example/example1.bibcode)
