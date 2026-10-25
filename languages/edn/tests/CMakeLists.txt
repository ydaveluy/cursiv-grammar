# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file under example/ parses clean.
add_executable(edn_grammar_test grammar_test.cpp)
target_link_libraries(edn_grammar_test PRIVATE ednCore)
target_compile_definitions(edn_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME edn_grammar_test COMMAND edn_grammar_test)

# ...and the command line reads one of them.
if(TARGET edn_cli)
  add_test(NAME edn_cli_test COMMAND edn_cli
           ${PROJECT_SOURCE_DIR}/example/deps.edn)
endif()
