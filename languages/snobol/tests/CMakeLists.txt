# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean, and two
# of them are read back field by field.
add_executable(snobol_grammar_test grammar_test.cpp)
target_link_libraries(snobol_grammar_test PRIVATE snobolCore)
target_compile_definitions(snobol_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME snobol_grammar_test COMMAND snobol_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET snobol_cli)
  add_test(NAME snobol_cli_test COMMAND snobol_cli
           ${PROJECT_SOURCE_DIR}/example/hello.sno)
endif()
