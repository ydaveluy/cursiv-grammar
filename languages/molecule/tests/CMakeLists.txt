# The grammar itself: every example/*.mol parses clean into a Molecule.
add_executable(molecule_grammar_test grammar_test.cpp)
target_link_libraries(molecule_grammar_test PRIVATE moleculeCore)
target_compile_definitions(molecule_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME molecule_grammar_test COMMAND molecule_grammar_test)

# ...and the command line reads one of them.
add_test(NAME molecule_cli_test COMMAND molecule_cli
         ${PROJECT_SOURCE_DIR}/example/CCl4.mol)
