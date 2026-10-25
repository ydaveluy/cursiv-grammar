# The grammar itself: every document under example/ parses clean.
add_executable(sqlite_grammar_test grammar_test.cpp)
target_link_libraries(sqlite_grammar_test PRIVATE sqliteCore)
target_compile_definitions(sqlite_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sqlite_grammar_test COMMAND sqlite_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME sqlite_cli_test COMMAND sqlite_cli
         ${PROJECT_SOURCE_DIR}/example/identifiers.sql)
