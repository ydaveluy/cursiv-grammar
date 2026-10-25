# The grammar itself: every example parses clean, and the tree holds what it
# declares.
add_executable(corundum_grammar_test grammar_test.cpp)
target_link_libraries(corundum_grammar_test PRIVATE corundumCore)
target_compile_definitions(corundum_grammar_test PRIVATE
    CURSIV_SAMPLE="${PROJECT_SOURCE_DIR}/example/test.rb"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME corundum_grammar_test COMMAND corundum_grammar_test)

# ...and the command line reads the same sample.
add_test(NAME corundum_cli_test COMMAND corundum_cli
         ${PROJECT_SOURCE_DIR}/example/test.rb)
