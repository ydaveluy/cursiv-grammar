# The grammar itself: the upstream corpus and a hand-written sample of this
# port's own, kept separate from `example/` (the campaign's verifier counts
# that directory as the upstream corpus).
add_executable(stringtemplate_grammar_test grammar_test.cpp)
target_link_libraries(stringtemplate_grammar_test PRIVATE stringtemplateCore cursiv::test)
target_compile_definitions(stringtemplate_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME stringtemplate_grammar_test COMMAND stringtemplate_grammar_test)

# ...and the command line reads the upstream sample.
add_test(NAME stringtemplate_cli_test COMMAND stringtemplate_cli
         ${PROJECT_SOURCE_DIR}/example/example1.st)
