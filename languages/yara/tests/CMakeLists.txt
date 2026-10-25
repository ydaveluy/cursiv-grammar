add_executable(yara_grammar_test grammar_test.cpp)
target_link_libraries(yara_grammar_test PRIVATE yaraCore)
target_compile_definitions(yara_grammar_test PRIVATE
    CURSIV_SAMPLE="${PROJECT_SOURCE_DIR}/example/test.yar"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME yara_grammar_test COMMAND yara_grammar_test)

add_test(NAME yara_cli_test COMMAND yara_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/native_valid.yar)

add_executable(yara_integer_test integer_test.cpp)
target_link_libraries(yara_integer_test PRIVATE yaraCore cursiv::test)
target_compile_definitions(yara_integer_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME yara_integer_test COMMAND yara_integer_test)
