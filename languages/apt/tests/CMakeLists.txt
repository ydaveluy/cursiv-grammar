# The grammar itself: every example under example/ parses clean.
add_executable(apt_grammar_test grammar_test.cpp)
target_link_libraries(apt_grammar_test PRIVATE aptCore)
target_compile_definitions(apt_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME apt_grammar_test COMMAND apt_grammar_test)

# ...and the command line reads one of the same files.
add_test(NAME apt_cli_test COMMAND apt_cli
         ${PROJECT_SOURCE_DIR}/tests/basic.list)

add_executable(apt_native_test native_test.cpp)
target_link_libraries(apt_native_test PRIVATE aptCore cursiv::test)
target_compile_definitions(apt_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME apt_native_test COMMAND apt_native_test)
