# The grammar itself: every sample under example/ parses, and each tree
# holds a Filter.
add_executable(rfc1960_grammar_test grammar_test.cpp)
target_link_libraries(rfc1960_grammar_test PRIVATE rfc1960Core)
target_compile_definitions(rfc1960_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1960_grammar_test COMMAND rfc1960_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME rfc1960_cli_test COMMAND rfc1960_cli
         ${PROJECT_SOURCE_DIR}/example/example3.rfc1960)

add_executable(rfc1960_native_test native_test.cpp)
target_link_libraries(rfc1960_native_test PRIVATE rfc1960Core cursiv::test)
target_compile_definitions(rfc1960_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1960_native_test COMMAND rfc1960_native_test)
