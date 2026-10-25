# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: what this language takes, and what it refuses.
add_executable(geekcode_grammar_test grammar_test.cpp)
target_link_libraries(geekcode_grammar_test PRIVATE geekcodeCore cursiv::test)
target_compile_definitions(geekcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME geekcode_grammar_test COMMAND geekcode_grammar_test)

# ...and the command line reads one corpus document.
if(TARGET geekcode_cli)
  add_test(NAME geekcode_cli_test COMMAND geekcode_cli
           ${PROJECT_SOURCE_DIR}/example/example1.geek)
endif()
