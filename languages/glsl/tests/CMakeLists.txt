# The grammar itself: every document under example/ parses clean.
add_executable(glsl_grammar_test grammar_test.cpp)
target_link_libraries(glsl_grammar_test PRIVATE glslCore cursiv::test)
target_compile_definitions(glsl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME glsl_grammar_test COMMAND glsl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME glsl_cli_test COMMAND glsl_cli
         ${PROJECT_SOURCE_DIR}/example/simpleFunctionCall.frag)
