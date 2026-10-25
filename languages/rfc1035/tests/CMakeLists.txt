# The grammar itself: every document under example/ parses clean.
add_executable(rfc1035_grammar_test grammar_test.cpp)
target_link_libraries(rfc1035_grammar_test PRIVATE rfc1035Core)
target_compile_definitions(rfc1035_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1035_grammar_test COMMAND rfc1035_grammar_test)

# ...and the command line reads one of them.
add_test(NAME rfc1035_cli_test COMMAND rfc1035_cli
         ${PROJECT_SOURCE_DIR}/example/example1.rfc1035)

add_executable(rfc1035_native_test native_test.cpp)
target_link_libraries(rfc1035_native_test PRIVATE rfc1035Core cursiv::test)
target_compile_definitions(rfc1035_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1035_native_test COMMAND rfc1035_native_test)
