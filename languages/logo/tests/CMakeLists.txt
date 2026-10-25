# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses (or fails exactly
# where its own name declares it will), and a few field values read back.
add_executable(logo_grammar_test grammar_test.cpp)
target_link_libraries(logo_grammar_test PRIVATE logoCore)
target_compile_definitions(logo_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME logo_grammar_test COMMAND logo_grammar_test)

# ...and the command line reads one clean document from the same corpus.
if(TARGET logo_cli)
  add_test(NAME logo_cli_test COMMAND logo_cli
           ${PROJECT_SOURCE_DIR}/example/repeat.logo)
endif()
