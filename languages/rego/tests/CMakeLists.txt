# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean and
# builds a Document; a second, inline document checks field readback.
add_executable(rego_grammar_test grammar_test.cpp)
target_link_libraries(rego_grammar_test PRIVATE regoCore cursiv::test)
target_compile_definitions(rego_grammar_test PRIVATE
    "CURSIV_EXAMPLE_DIR=\"${PROJECT_SOURCE_DIR}/example\""
    "CURSIV_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME rego_grammar_test COMMAND rego_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET rego_cli)
  add_test(NAME rego_cli_test COMMAND rego_cli
           ${PROJECT_SOURCE_DIR}/example/01725.rego)
endif()
