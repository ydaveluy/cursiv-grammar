# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: what this language takes, and what it refuses.
add_executable(spl_grammar_test grammar_test.cpp)
target_link_libraries(spl_grammar_test PRIVATE splCore)
target_compile_definitions(spl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME spl_grammar_test COMMAND spl_grammar_test)

# ...and the command line reads one corpus file.
if(TARGET spl_cli)
  add_test(NAME spl_cli_test COMMAND spl_cli
           ${PROJECT_SOURCE_DIR}/example/simple_query.spl)
endif()
