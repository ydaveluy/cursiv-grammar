# The grammar itself: every corpus document parses clean, and res_proof.spass
# is read back field by field.
add_executable(spass_grammar_test grammar_test.cpp)
target_link_libraries(spass_grammar_test PRIVATE spassCore)
target_compile_definitions(spass_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.spass")
add_test(NAME spass_grammar_test COMMAND spass_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME spass_cli_test COMMAND spass_cli
         ${PROJECT_SOURCE_DIR}/example/res_proof.spass)
