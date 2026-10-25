# The grammar itself: every example/ document parses, and the tree holds a
# Font.
add_executable(bdf_grammar_test grammar_test.cpp)
target_link_libraries(bdf_grammar_test PRIVATE bdfCore)
target_compile_definitions(bdf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME bdf_grammar_test COMMAND bdf_grammar_test)

# ...and the command line reads one of them.
add_test(NAME bdf_cli_test COMMAND bdf_cli
         ${PROJECT_SOURCE_DIR}/example/example1.bdf)

add_executable(bdf_native_test native_test.cpp)
target_link_libraries(bdf_native_test PRIVATE bdfCore cursiv::test)
target_compile_definitions(bdf_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bdf_native_test COMMAND bdf_native_test)
