# The upstream corpus ships no examples/ at all, so every sample lives
# here rather than under example/.

add_executable(atl_grammar_test grammar_test.cpp)
target_link_libraries(atl_grammar_test PRIVATE atlCore cursiv::test cursiv::lsp)
target_compile_definitions(atl_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME atl_grammar_test COMMAND atl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME atl_cli_test COMMAND atl_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/module.atl)
