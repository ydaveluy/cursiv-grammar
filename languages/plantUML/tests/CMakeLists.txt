# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses clean.
add_executable(plantuml_grammar_test grammar_test.cpp)
target_link_libraries(plantuml_grammar_test PRIVATE plantumlCore)
target_compile_definitions(plantuml_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME plantuml_grammar_test COMMAND plantuml_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET plantuml_cli)
  add_test(NAME plantuml_cli_test COMMAND plantuml_cli
           ${PROJECT_SOURCE_DIR}/example/x7.plantuml)
endif()
