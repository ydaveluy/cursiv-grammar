# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `moleculeGenerated` (the compiled library) and `molecule_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID molecule
    GRAMMAR    molecule/core/MoleculeGrammar.hpp
    NAMESPACE  molecule::Molecule
    EXTENSIONS .mol
    OUT        molecule/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/molecule/core/*.cpp)
add_library(moleculeCore ${_core})
target_link_libraries(moleculeCore PUBLIC moleculeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/molecule/cli/*.cpp)
add_executable(molecule_cli ${_cli})
target_link_libraries(molecule_cli
                      PRIVATE moleculeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/molecule/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/molecule/lsp/main.cpp)
add_library(moleculeLsp ${_lsp})
target_link_libraries(moleculeLsp PUBLIC moleculeCore cursiv::lsp)
add_executable(molecule_lsp molecule/lsp/main.cpp)
target_link_libraries(molecule_lsp PRIVATE moleculeLsp)

cursiv_add_benchmark(molecule SOURCES molecule/bench/main.cpp)

cursiv_package_language(molecule)
