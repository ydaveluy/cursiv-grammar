# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses the way its corpus says
# it should, and a handful of real field values read back correctly.
add_executable(golang_grammar_test grammar_test.cpp)
target_link_libraries(golang_grammar_test PRIVATE golangCore)
target_compile_definitions(golang_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/corpus")
add_test(NAME golang_grammar_test COMMAND golang_grammar_test)

# ...and the command line reads one clean sample.
if(TARGET golang_cli)
  add_test(NAME golang_cli_test COMMAND golang_cli
           ${PROJECT_SOURCE_DIR}/example/example.go)
endif()
