# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.pdn)
list(JOIN _samples "," _samples_joined)

# black-win.pdn lives beside this test, not in the corpus: "0-1" (Black
# wins) is the one result value no upstream sample uses.
file(GLOB _test_samples CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.pdn)
list(JOIN _test_samples "," _test_samples_joined)

# The grammar itself: every sample parses, and the tree holds a Game.
add_executable(pdn_grammar_test grammar_test.cpp)
target_link_libraries(pdn_grammar_test PRIVATE pdnCore)
target_compile_definitions(pdn_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\""
    "CURSIV_TEST_SAMPLES=\"${_test_samples_joined}\"")
add_test(NAME pdn_grammar_test COMMAND pdn_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET pdn_cli)
  add_test(NAME pdn_cli_test COMMAND pdn_cli
           ${PROJECT_SOURCE_DIR}/example/example2.pdn)
endif()
