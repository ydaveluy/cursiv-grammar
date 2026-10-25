# The upstream corpus ships no examples/ at all, so every document lives
# here rather than under example/.

add_executable(fusiontables_grammar_test grammar_test.cpp)
target_link_libraries(fusiontables_grammar_test PRIVATE fusiontablesCore)
target_compile_definitions(fusiontables_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fusiontables_grammar_test COMMAND fusiontables_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME fusiontables_cli_test COMMAND fusiontables_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/select.fql)
