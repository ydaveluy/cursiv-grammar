# The grammar itself: every corpus document parses clean, features.mod
# covers what the corpus never reaches, and hello.mod is read back field by
# field.
add_executable(oberon_grammar_test grammar_test.cpp)
target_link_libraries(oberon_grammar_test PRIVATE oberonCore)
target_compile_definitions(oberon_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.mod")
add_test(NAME oberon_grammar_test COMMAND oberon_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME oberon_cli_test COMMAND oberon_cli
         ${PROJECT_SOURCE_DIR}/example/hello.mod)
