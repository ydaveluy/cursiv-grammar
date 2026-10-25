# The grammar itself: every example parses, and each tree holds a Model.
add_executable(scotty_grammar_test grammar_test.cpp)
target_link_libraries(scotty_grammar_test PRIVATE scottyCore)
target_compile_definitions(scotty_grammar_test PRIVATE
    CURSIV_EXAMPLE_1="${PROJECT_SOURCE_DIR}/example/example1.scotty"
    CURSIV_EXAMPLE_2="${CMAKE_CURRENT_SOURCE_DIR}/demo.scotty")
add_test(NAME scotty_grammar_test COMMAND scotty_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME scotty_cli_test COMMAND scotty_cli
         ${PROJECT_SOURCE_DIR}/example/example1.scotty)
