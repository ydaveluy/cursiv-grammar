# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(gff3_grammar_test grammar_test.cpp)
target_link_libraries(gff3_grammar_test PRIVATE gff3Core)
target_compile_definitions(gff3_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gff3_grammar_test COMMAND gff3_grammar_test)

# ...and the command line reads one example.
if(TARGET gff3_cli)
  add_test(NAME gff3_cli_test COMMAND gff3_cli
           ${PROJECT_SOURCE_DIR}/example/NC_001792.gff)
endif()

add_executable(gff3_native_test native_test.cpp)
target_link_libraries(gff3_native_test PRIVATE gff3Core cursiv::test)
target_compile_definitions(gff3_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gff3_native_test COMMAND gff3_native_test)
