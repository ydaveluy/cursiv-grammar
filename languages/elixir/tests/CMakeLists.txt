add_executable(elixir_grammar_test grammar_test.cpp)
target_link_libraries(elixir_grammar_test PRIVATE elixirCore)
target_compile_definitions(elixir_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME elixir_grammar_test COMMAND elixir_grammar_test)

# ...and the command line reads one sample of the corpus.
add_test(NAME elixir_cli_test COMMAND elixir_cli
         ${PROJECT_SOURCE_DIR}/example/list.ex)
