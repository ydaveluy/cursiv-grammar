# The grammar itself: every sample of example/ parses as expected, plus
# this port's own document exercising constructs the corpus does not reach.
add_executable(asn_grammar_test grammar_test.cpp)
target_link_libraries(asn_grammar_test PRIVATE asnCore)
target_compile_definitions(asn_grammar_test PRIVATE
    CURSIV_EXAMPLE1_SAMPLE="${PROJECT_SOURCE_DIR}/example/example1.asn"
    CURSIV_EXAMPLE2_SAMPLE="${PROJECT_SOURCE_DIR}/example/example2.asn"
    CURSIV_CONSTRUCTS_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/constructs.asn")
add_test(NAME asn_grammar_test COMMAND asn_grammar_test)

# ...and the command line reads one of the clean samples.
add_test(NAME asn_cli_test COMMAND asn_cli
         ${PROJECT_SOURCE_DIR}/example/example1.asn)
