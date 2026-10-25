# The grammar itself: every document under example/ parses clean.
add_executable(capnproto_grammar_test grammar_test.cpp)
target_link_libraries(capnproto_grammar_test PRIVATE capnprotoCore)
target_compile_definitions(capnproto_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME capnproto_grammar_test COMMAND capnproto_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME capnproto_cli_test COMMAND capnproto_cli
         ${PROJECT_SOURCE_DIR}/example/addressbook.capnp)
