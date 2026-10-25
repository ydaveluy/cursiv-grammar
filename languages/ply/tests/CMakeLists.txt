# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(ply_grammar_test grammar_test.cpp)
target_link_libraries(ply_grammar_test PRIVATE plyCore)
target_compile_definitions(ply_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ply_grammar_test COMMAND ply_grammar_test)

# ...and the command line reads one example.
if(TARGET ply_cli)
  add_test(NAME ply_cli_test COMMAND ply_cli
           ${PROJECT_SOURCE_DIR}/example/example1.ply)
endif()

add_executable(ply_native_test native_test.cpp)
target_link_libraries(ply_native_test PRIVATE plyCore cursiv::test)
target_compile_definitions(ply_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ply_native_test COMMAND ply_native_test)
