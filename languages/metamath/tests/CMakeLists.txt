# The grammar itself: every document of example/ parses clean.
add_executable(metamath_grammar_test grammar_test.cpp)
target_link_libraries(metamath_grammar_test PRIVATE metamathCore)
target_compile_definitions(metamath_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME metamath_grammar_test COMMAND metamath_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME metamath_cli_test COMMAND metamath_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/demo.mm)
