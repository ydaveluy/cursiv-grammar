# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file under example/ parses clean.
add_executable(aterm_grammar_test grammar_test.cpp)
target_link_libraries(aterm_grammar_test PRIVATE atermCore)
target_compile_definitions(aterm_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME aterm_grammar_test COMMAND aterm_grammar_test)

# ...and the command line reads one of them.
if(TARGET aterm_cli)
  add_test(NAME aterm_cli_test COMMAND aterm_cli
           ${PROJECT_SOURCE_DIR}/example/example2.aterm)
endif()

add_executable(aterm_native_test native_test.cpp)
target_link_libraries(aterm_native_test PRIVATE atermCore cursiv::test)
target_compile_definitions(aterm_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME aterm_native_test COMMAND aterm_native_test)
