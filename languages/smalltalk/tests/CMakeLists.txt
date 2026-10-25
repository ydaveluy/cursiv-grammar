# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(smalltalk_grammar_test grammar_test.cpp)
target_link_libraries(smalltalk_grammar_test PRIVATE smalltalkCore)
target_compile_definitions(smalltalk_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME smalltalk_grammar_test COMMAND smalltalk_grammar_test)

# ...and the command line reads one sample.
if(TARGET smalltalk_cli)
  add_test(NAME smalltalk_cli_test COMMAND smalltalk_cli
           ${PROJECT_SOURCE_DIR}/example/helloworld.st)
endif()
