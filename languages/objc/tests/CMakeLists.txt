# The grammar itself: every example under example/ parses clean.
add_executable(objc_grammar_test grammar_test.cpp)
target_link_libraries(objc_grammar_test PRIVATE objcCore cursiv::test)
target_compile_definitions(objc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME objc_grammar_test COMMAND objc_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME objc_cli_test COMMAND objc_cli
         ${PROJECT_SOURCE_DIR}/example/Enums.m)
