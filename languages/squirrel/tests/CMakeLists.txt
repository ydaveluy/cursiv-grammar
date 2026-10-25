# The grammar itself: every example parses, and a few field values are read
# back out of the AST.
add_executable(squirrel_grammar_test grammar_test.cpp)
target_link_libraries(squirrel_grammar_test PRIVATE squirrelCore)
target_compile_definitions(squirrel_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME squirrel_grammar_test COMMAND squirrel_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME squirrel_cli_test COMMAND squirrel_cli
         ${PROJECT_SOURCE_DIR}/example/ackermann.nut)
