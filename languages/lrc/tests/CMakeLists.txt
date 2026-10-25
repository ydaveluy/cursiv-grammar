# The grammar itself: every example parses clean into a Document.
add_executable(lrc_grammar_test grammar_test.cpp)
target_link_libraries(lrc_grammar_test PRIVATE lrcCore)
target_compile_definitions(lrc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lrc_grammar_test COMMAND lrc_grammar_test)

# ...and the command line reads one example.
add_test(NAME lrc_cli_test COMMAND lrc_cli
         ${PROJECT_SOURCE_DIR}/example/example1.lrc)

add_executable(lrc_native_test native_test.cpp)
target_link_libraries(lrc_native_test PRIVATE lrcCore cursiv::test)
target_compile_definitions(lrc_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lrc_native_test COMMAND lrc_native_test)
