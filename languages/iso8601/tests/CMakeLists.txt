# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean and builds a
# Document.
add_executable(iso8601_grammar_test grammar_test.cpp)
target_link_libraries(iso8601_grammar_test PRIVATE iso8601Core)
target_compile_definitions(iso8601_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME iso8601_grammar_test COMMAND iso8601_grammar_test)

# ...and the command line reads one sample from the same corpus.
if(TARGET iso8601_cli)
  add_test(NAME iso8601_cli_test COMMAND iso8601_cli
           ${PROJECT_SOURCE_DIR}/example/b-1-3-cdlt-e.iso8601)
endif()

add_executable(iso8601_native_test native_test.cpp)
target_link_libraries(iso8601_native_test PRIVATE iso8601Core cursiv::test)
target_compile_definitions(iso8601_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME iso8601_native_test COMMAND iso8601_native_test)
