# The grammar itself: every document under tests/ (the upstream corpus is
# empty) parses clean, and real field values read back correctly.
add_executable(pddl_grammar_test grammar_test.cpp)
target_link_libraries(pddl_grammar_test PRIVATE pddlCore cursiv::test)
target_compile_definitions(pddl_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pddl_grammar_test COMMAND pddl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME pddl_cli_test COMMAND pddl_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/blocksworld-domain.pddl)
