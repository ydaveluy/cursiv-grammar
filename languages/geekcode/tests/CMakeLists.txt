# The grammar itself: what this language takes, and what it refuses.
add_executable(geekcode_grammar_test grammar_test.cpp)
target_link_libraries(geekcode_grammar_test PRIVATE geekcodeCore cursiv::test)
target_compile_definitions(geekcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME geekcode_grammar_test COMMAND geekcode_grammar_test)

# ...and the command line reads one corpus document.
add_test(NAME geekcode_cli_test COMMAND geekcode_cli
         ${PROJECT_SOURCE_DIR}/example/example1.geek)
