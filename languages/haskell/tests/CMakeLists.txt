file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.hs)
list(JOIN _samples "," _samples_joined)

# The grammar itself: every sample parses, and the tree holds a Module.
add_executable(haskell_grammar_test grammar_test.cpp)
target_link_libraries(haskell_grammar_test PRIVATE haskellCore)
target_compile_definitions(haskell_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\"")
add_test(NAME haskell_grammar_test COMMAND haskell_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME haskell_cli_test COMMAND haskell_cli
         ${PROJECT_SOURCE_DIR}/example/tree.hs)

add_executable(haskell_as_pattern_test as_pattern_test.cpp)
target_link_libraries(haskell_as_pattern_test PRIVATE haskellCore)
target_compile_definitions(haskell_as_pattern_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME haskell_as_pattern_test COMMAND haskell_as_pattern_test)
