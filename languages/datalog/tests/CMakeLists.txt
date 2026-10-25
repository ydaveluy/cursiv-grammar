file(GLOB _samples CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/example/*.dl
    ${CMAKE_CURRENT_SOURCE_DIR}/*.dl)
list(JOIN _samples "," _samples_joined)

# The grammar itself: every sample parses, and the tree holds a Program.
add_executable(datalog_grammar_test grammar_test.cpp)
target_link_libraries(datalog_grammar_test PRIVATE datalogCore)
target_compile_definitions(datalog_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\"")
add_test(NAME datalog_grammar_test COMMAND datalog_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME datalog_cli_test COMMAND datalog_cli
         ${PROJECT_SOURCE_DIR}/example/example1.dl)
