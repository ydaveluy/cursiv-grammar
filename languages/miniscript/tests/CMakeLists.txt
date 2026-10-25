# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.ms
     ${CMAKE_CURRENT_SOURCE_DIR}/*.ms)
list(JOIN _samples "," _samples_joined)

# The grammar itself: every sample parses clean and builds a Document.
add_executable(miniscript_grammar_test grammar_test.cpp)
target_link_libraries(miniscript_grammar_test PRIVATE miniscriptCore)
target_compile_definitions(miniscript_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\""
    "CURSIV_SAMPLE_DIR=\"${PROJECT_SOURCE_DIR}/example\"")
add_test(NAME miniscript_grammar_test COMMAND miniscript_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET miniscript_cli)
  add_test(NAME miniscript_cli_test COMMAND miniscript_cli
           ${PROJECT_SOURCE_DIR}/example/basic.ms)
endif()
