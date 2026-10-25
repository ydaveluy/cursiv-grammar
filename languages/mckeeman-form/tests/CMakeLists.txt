# The grammar itself: every document under example/ parses, and each holds
# what it declares.
add_executable(mckeemanform_grammar_test grammar_test.cpp)
target_link_libraries(mckeemanform_grammar_test PRIVATE mckeemanformCore cursiv::test)
target_compile_definitions(mckeemanform_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME mckeemanform_grammar_test COMMAND mckeemanform_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME mckeemanform_cli_test COMMAND mckeemanform_cli
         ${PROJECT_SOURCE_DIR}/example/mckeeman.mckeemanform)
