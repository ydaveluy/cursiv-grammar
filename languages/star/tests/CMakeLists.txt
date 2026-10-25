# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(star_grammar_test grammar_test.cpp)
target_link_libraries(star_grammar_test PRIVATE starCore)
target_compile_definitions(star_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME star_grammar_test COMMAND star_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET star_cli)
  add_test(NAME star_cli_test COMMAND star_cli
           ${PROJECT_SOURCE_DIR}/example/example1.star)
endif()

add_executable(star_native_test native_test.cpp)
target_link_libraries(star_native_test PRIVATE starCore cursiv::test)
target_compile_definitions(star_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME star_native_test COMMAND star_native_test)
