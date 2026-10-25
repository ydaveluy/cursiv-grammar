# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(smtlibv2_grammar_test grammar_test.cpp)
target_link_libraries(smtlibv2_grammar_test PRIVATE smtlibv2Core)
target_compile_definitions(smtlibv2_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME smtlibv2_grammar_test COMMAND smtlibv2_grammar_test)

if(TARGET smtlibv2_cli)
  add_test(NAME smtlibv2_cli_test COMMAND smtlibv2_cli
           ${PROJECT_SOURCE_DIR}/example/test1.smt2)
endif()
