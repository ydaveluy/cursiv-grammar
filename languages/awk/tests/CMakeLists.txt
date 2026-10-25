# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(awk_grammar_test grammar_test.cpp)
target_link_libraries(awk_grammar_test PRIVATE awkCore)
target_compile_definitions(awk_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME awk_grammar_test COMMAND awk_grammar_test)

add_executable(awk_native_regressions native_regressions.cpp)
target_link_libraries(awk_native_regressions PRIVATE awkCore cursiv::test)
add_test(NAME awk_native_regressions COMMAND awk_native_regressions)

if(TARGET awk_cli)
  add_test(NAME awk_cli_test COMMAND awk_cli
           ${PROJECT_SOURCE_DIR}/example/count_words.awk)
endif()
