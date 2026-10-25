# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(memcachedprotocol_grammar_test grammar_test.cpp)
target_link_libraries(memcachedprotocol_grammar_test PRIVATE memcachedprotocolCore)
target_compile_definitions(memcachedprotocol_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME memcachedprotocol_grammar_test COMMAND memcachedprotocol_grammar_test)

# ...and the command line reads one of them.
add_test(NAME memcachedprotocol_cli_test COMMAND memcachedprotocol_cli
         ${PROJECT_SOURCE_DIR}/example/example1.memcachedprotocol)
