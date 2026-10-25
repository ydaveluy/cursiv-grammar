# The grammar itself: every document under example/ parses clean.
add_executable(rfc3080_grammar_test grammar_test.cpp)
target_link_libraries(rfc3080_grammar_test PRIVATE rfc3080Core cursiv::test)
target_compile_definitions(rfc3080_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc3080_grammar_test COMMAND rfc3080_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME rfc3080_cli_test COMMAND rfc3080_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/msg.beep)

add_test(NAME rfc3080_invalid_frame_test COMMAND rfc3080_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/invalid-size/msg.beep)
set_tests_properties(rfc3080_invalid_frame_test PROPERTIES WILL_FAIL TRUE)

add_test(NAME rfc3080_embedded_end_test COMMAND rfc3080_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/native-valid.beep)
