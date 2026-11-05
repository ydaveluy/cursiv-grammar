# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID molecule
    GRAMMAR    molecule/MoleculeGrammar.hpp
    NAMESPACE  molecule::Molecule
    EXTENSIONS .mol
    OUT        molecule/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(moleculeCore ${_core})
target_link_libraries(moleculeCore PUBLIC moleculeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(molecule_cli cli.cpp)
  target_include_directories(molecule_cli PRIVATE cli)
  target_link_libraries(molecule_cli PRIVATE moleculeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(moleculeCli ${_cli})
    target_include_directories(moleculeCli PUBLIC cli)
    target_link_libraries(moleculeCli PUBLIC moleculeCore cursiv::cli)
    target_link_libraries(molecule_cli PRIVATE moleculeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(moleculeLsp ${_lsp})
  target_include_directories(moleculeLsp PUBLIC lsp)
  target_link_libraries(moleculeLsp PUBLIC moleculeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(molecule_lsp lsp.cpp)
    target_link_libraries(molecule_lsp PRIVATE moleculeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET moleculeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(molecule SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(molecule)
