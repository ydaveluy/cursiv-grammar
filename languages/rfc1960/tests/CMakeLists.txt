# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample under example/ parses, and each tree
# holds a Filter.
add_executable(rfc1960_grammar_test grammar_test.cpp)
target_link_libraries(rfc1960_grammar_test PRIVATE rfc1960Core)
target_compile_definitions(rfc1960_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1960_grammar_test COMMAND rfc1960_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET rfc1960_cli)
  add_test(NAME rfc1960_cli_test COMMAND rfc1960_cli
           ${PROJECT_SOURCE_DIR}/example/example3.rfc1960)
endif()

add_executable(rfc1960_native_test native_test.cpp)
target_link_libraries(rfc1960_native_test PRIVATE rfc1960Core cursiv::test)
target_compile_definitions(rfc1960_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc1960_native_test COMMAND rfc1960_native_test)
