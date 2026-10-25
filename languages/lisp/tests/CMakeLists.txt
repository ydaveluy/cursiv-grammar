# The grammar itself: every document in example/ parses clean, and builds a Doc.
add_executable(lisp_grammar_test grammar_test.cpp)
target_link_libraries(lisp_grammar_test PRIVATE lispCore)
target_compile_definitions(lisp_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lisp_grammar_test COMMAND lisp_grammar_test)

# ...and the command line reads one of them.
add_test(NAME lisp_cli_test COMMAND lisp_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/lists.lisp)
