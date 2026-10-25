# The grammar itself: every example parses clean, and the tree holds a
# compilation unit.
add_executable(csharp_grammar_test grammar_test.cpp)
target_link_libraries(csharp_grammar_test PRIVATE csharpCore)
target_compile_definitions(csharp_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME csharp_grammar_test COMMAND csharp_grammar_test)

# ...and the command line reads one example.
add_test(NAME csharp_cli_test COMMAND csharp_cli
         ${PROJECT_SOURCE_DIR}/example/AllInOneNoPreprocessor.cs)
