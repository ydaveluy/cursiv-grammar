# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Schema.
add_executable(flatbuffers_grammar_test grammar_test.cpp)
target_link_libraries(flatbuffers_grammar_test PRIVATE flatbuffersCore)
target_compile_definitions(flatbuffers_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME flatbuffers_grammar_test COMMAND flatbuffers_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET flatbuffers_cli)
  add_test(NAME flatbuffers_cli_test COMMAND flatbuffers_cli
           ${PROJECT_SOURCE_DIR}/example/monster.fbs)
endif()

add_executable(flatbuffers_metadata_test metadata_test.cpp)
target_link_libraries(flatbuffers_metadata_test PRIVATE flatbuffersCore)
target_compile_definitions(flatbuffers_metadata_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME flatbuffers_metadata_test COMMAND flatbuffers_metadata_test)
