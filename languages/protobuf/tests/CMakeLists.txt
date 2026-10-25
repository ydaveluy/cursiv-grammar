# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: a few shapes exercised directly, and the whole corpus.
add_executable(protobuf_grammar_test grammar_test.cpp)
target_link_libraries(protobuf_grammar_test PRIVATE protobufCore)
target_compile_definitions(protobuf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME protobuf_grammar_test COMMAND protobuf_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET protobuf_cli)
  add_test(NAME protobuf_cli_test COMMAND protobuf_cli
           ${PROJECT_SOURCE_DIR}/example/helloworld.proto)
endif()

add_executable(protobuf_number_test number_test.cpp)
target_link_libraries(protobuf_number_test PRIVATE protobufCore cursiv::test)
target_compile_definitions(protobuf_number_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME protobuf_number_test COMMAND protobuf_number_test)
