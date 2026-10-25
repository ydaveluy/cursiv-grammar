# The grammar itself: every document in example/ parses, and the tree holds
# a SearchCrit.
add_executable(upnp_grammar_test grammar_test.cpp)
target_link_libraries(upnp_grammar_test PRIVATE upnpCore)
target_compile_definitions(upnp_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME upnp_grammar_test COMMAND upnp_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME upnp_cli_test COMMAND upnp_cli
         ${PROJECT_SOURCE_DIR}/example/search1.upnp)

add_executable(upnp_native_values_test native_values_test.cpp)
target_link_libraries(upnp_native_values_test PRIVATE upnpCore cursiv::test)
add_test(NAME upnp_native_values_test COMMAND upnp_native_values_test)
