# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID smiles
    GRAMMAR    smiles/SmilesGrammar.hpp
    NAMESPACE  smiles::Smiles
    EXTENSIONS .smi
    OUT        smiles/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(smilesCore ${_core})
target_link_libraries(smilesCore PUBLIC smilesGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(smiles_cli cli.cpp)
  target_include_directories(smiles_cli PRIVATE cli)
  target_link_libraries(smiles_cli PRIVATE smilesCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(smilesCli ${_cli})
    target_include_directories(smilesCli PUBLIC cli)
    target_link_libraries(smilesCli PUBLIC smilesCore cursiv::cli)
    target_link_libraries(smiles_cli PRIVATE smilesCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(smilesLsp ${_lsp})
  target_include_directories(smilesLsp PUBLIC lsp)
  target_link_libraries(smilesLsp PUBLIC smilesCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(smiles_lsp lsp.cpp)
    target_link_libraries(smiles_lsp PRIVATE smilesLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET smilesLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(smiles SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(smiles)
