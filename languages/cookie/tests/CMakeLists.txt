# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(cookie_grammar_test grammar_test.cpp)
target_link_libraries(cookie_grammar_test PRIVATE cookieCore cursiv::test)
target_compile_definitions(cookie_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cookie_grammar_test COMMAND cookie_grammar_test)

# ...and the command line reads one of them.
add_test(NAME cookie_cli_test COMMAND cookie_cli
         ${PROJECT_SOURCE_DIR}/example/example1.cookie)
