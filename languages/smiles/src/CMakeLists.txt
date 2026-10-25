# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `smilesGenerated` (the compiled library) and `smiles_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID smiles
    GRAMMAR    smiles/core/SmilesGrammar.hpp
    NAMESPACE  smiles::Smiles
    EXTENSIONS .smi
    OUT        smiles/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smiles/core/*.cpp)
add_library(smilesCore ${_core})
target_link_libraries(smilesCore PUBLIC smilesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smiles/cli/*.cpp)
add_executable(smiles_cli ${_cli})
target_link_libraries(smiles_cli
                      PRIVATE smilesCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smiles/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smiles/lsp/main.cpp)
add_library(smilesLsp ${_lsp})
target_link_libraries(smilesLsp PUBLIC smilesCore cursiv::lsp)
add_executable(smiles_lsp smiles/lsp/main.cpp)
target_link_libraries(smiles_lsp PRIVATE smilesLsp)

cursiv_add_benchmark(smiles SOURCES smiles/bench/main.cpp)

cursiv_package_language(smiles)
