# The grammar itself: every document under example/ parses clean.
add_executable(rcs_grammar_test grammar_test.cpp)
target_link_libraries(rcs_grammar_test PRIVATE rcsCore)
target_compile_definitions(rcs_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rcs_grammar_test COMMAND rcs_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME rcs_cli_test COMMAND rcs_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/full.rcs)

add_executable(rcs_native_test native_test.cpp)
target_link_libraries(rcs_native_test PRIVATE rcsCore cursiv::test)
target_compile_definitions(rcs_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rcs_native_test COMMAND rcs_native_test)
