# The grammar itself: every document under example/ and tests/ parses, and
# the tree holds what it declares.
add_executable(xpath_grammar_test grammar_test.cpp)
target_link_libraries(xpath_grammar_test PRIVATE xpathCore)
target_compile_definitions(xpath_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME xpath_grammar_test COMMAND xpath_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME xpath_cli_test COMMAND xpath_cli
         ${PROJECT_SOURCE_DIR}/example/expressions_001.xpath)

add_executable(xpath_native_values_test native_values_test.cpp)
target_link_libraries(xpath_native_values_test PRIVATE xpathCore cursiv::test)
add_test(NAME xpath_native_values_test COMMAND xpath_native_values_test)
