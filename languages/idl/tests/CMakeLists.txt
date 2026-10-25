# The grammar itself: every sample of example/ parses as expected, plus
# this port's own document exercising constructs the corpus does not reach.
add_executable(idl_grammar_test grammar_test.cpp)
target_link_libraries(idl_grammar_test PRIVATE idlCore)
target_compile_definitions(idl_grammar_test PRIVATE
    CURSIV_HELLOWORLD_SAMPLE="${PROJECT_SOURCE_DIR}/example/helloworld.idl"
    CURSIV_TESTIDLV4_SAMPLE="${PROJECT_SOURCE_DIR}/example/TestIDLv4.idl"
    CURSIV_FULL_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/full.idl")
add_test(NAME idl_grammar_test COMMAND idl_grammar_test)

# ...and the command line reads one of the clean samples.
add_test(NAME idl_cli_test COMMAND idl_cli
         ${PROJECT_SOURCE_DIR}/example/helloworld.idl)
