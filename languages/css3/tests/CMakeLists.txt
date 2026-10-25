# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, and two are read back
# field by field.
add_executable(css3_grammar_test grammar_test.cpp)
target_link_libraries(css3_grammar_test PRIVATE css3Core)
target_compile_definitions(css3_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME css3_grammar_test COMMAND css3_grammar_test)

# ...and the command line reads one of them.
if(TARGET css3_cli)
  add_test(NAME css3_cli_test COMMAND css3_cli
           ${PROJECT_SOURCE_DIR}/example/example1.css)
endif()
