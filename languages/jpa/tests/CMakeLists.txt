# The grammar itself: every document under example/ and tests/ parses clean.
add_executable(jpa_grammar_test grammar_test.cpp)
target_link_libraries(jpa_grammar_test PRIVATE jpaCore)
target_compile_definitions(jpa_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME jpa_grammar_test COMMAND jpa_grammar_test)

# ...and the command line reads one of the same documents. example1.jpql,
# the only corpus file, is the known-invalid one (see grammar_test.cpp), so
# this points at a document of our own instead.
add_test(NAME jpa_cli_test COMMAND jpa_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/coverage_select.jpql)
