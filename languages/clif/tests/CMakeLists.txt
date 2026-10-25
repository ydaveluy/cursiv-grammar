# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample of example/ parses as expected.
add_executable(clif_grammar_test grammar_test.cpp)
target_link_libraries(clif_grammar_test PRIVATE clifCore)
target_compile_definitions(clif_grammar_test PRIVATE
    CURSIV_MODULE_SAMPLE="${PROJECT_SOURCE_DIR}/example/module.clif"
    CURSIV_LOGIC_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/logic.clif")
add_test(NAME clif_grammar_test COMMAND clif_grammar_test)

# ...and the command line reads one of the clean samples.
if(TARGET clif_cli)
  add_test(NAME clif_cli_test COMMAND clif_cli
           ${PROJECT_SOURCE_DIR}/example/module.clif)
endif()
