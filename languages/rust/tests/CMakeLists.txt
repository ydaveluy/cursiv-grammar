# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.rs)
file(GLOB _own_samples CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.rs)
list(APPEND _samples ${_own_samples})
list(JOIN _samples "," _samples_joined)

# The grammar itself: every sample parses, and the tree holds a Crate.
add_executable(rust_grammar_test grammar_test.cpp)
target_link_libraries(rust_grammar_test PRIVATE rustCore)
target_compile_definitions(rust_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\"")
add_test(NAME rust_grammar_test COMMAND rust_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET rust_cli)
  add_test(NAME rust_cli_test COMMAND rust_cli
           ${PROJECT_SOURCE_DIR}/example/hello.rs)
endif()
