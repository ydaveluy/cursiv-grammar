# The grammar itself: every example parses, and the tree it builds is rooted
# in a proposition.
add_executable(propcalc_grammar_test grammar_test.cpp)
target_link_libraries(propcalc_grammar_test PRIVATE propcalcCore cursiv::test)
target_compile_definitions(propcalc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME propcalc_grammar_test COMMAND propcalc_grammar_test)

# ...and the command line reads one of them.
add_test(NAME propcalc_cli_test COMMAND propcalc_cli
         ${PROJECT_SOURCE_DIR}/example/modusponens.propcalc)
