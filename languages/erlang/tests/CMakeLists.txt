# The grammar itself: every file of example/ parses clean.
add_executable(erlang_grammar_test grammar_test.cpp)
target_link_libraries(erlang_grammar_test PRIVATE erlangCore)
target_compile_definitions(erlang_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME erlang_grammar_test COMMAND erlang_grammar_test)

# ...and the command line reads one sample of the corpus.
add_test(NAME erlang_cli_test COMMAND erlang_cli
         ${PROJECT_SOURCE_DIR}/example/fac.erl)
