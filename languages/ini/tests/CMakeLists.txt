# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()


add_executable(ini_grammar_test grammar_test.cpp)
target_link_libraries(ini_grammar_test PRIVATE iniCore cursiv::test)
target_compile_definitions(ini_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME ini_grammar_test COMMAND ini_grammar_test)

if(TARGET ini_cli)
  add_test(NAME ini_cli_test COMMAND ini_cli
           ${PROJECT_SOURCE_DIR}/example/config.ini)
endif()

add_executable(ini_fuzz_test fuzz_test.cpp)
target_link_libraries(ini_fuzz_test PRIVATE iniCore)
target_compile_definitions(ini_fuzz_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME ini_fuzz_test COMMAND ini_fuzz_test)

