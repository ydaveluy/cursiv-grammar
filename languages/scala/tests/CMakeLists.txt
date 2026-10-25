# The grammar itself: every document under example/ parses, recovery-free.
add_executable(scala_grammar_test grammar_test.cpp)
target_link_libraries(scala_grammar_test PRIVATE scalaCore)
target_compile_definitions(scala_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME scala_grammar_test COMMAND scala_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME scala_cli_test COMMAND scala_cli
         ${PROJECT_SOURCE_DIR}/example/class.scala)
