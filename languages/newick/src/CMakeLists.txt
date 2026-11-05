# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID newick
    GRAMMAR    newick/NewickGrammar.hpp
    NAMESPACE  newick::Newick
    EXTENSIONS .nwk
    OUT        newick/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(newickCore ${_core})
target_link_libraries(newickCore PUBLIC newickGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(newick_cli cli.cpp)
  target_include_directories(newick_cli PRIVATE cli)
  target_link_libraries(newick_cli PRIVATE newickCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(newickCli ${_cli})
    target_include_directories(newickCli PUBLIC cli)
    target_link_libraries(newickCli PUBLIC newickCore cursiv::cli)
    target_link_libraries(newick_cli PRIVATE newickCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(newickLsp ${_lsp})
  target_include_directories(newickLsp PUBLIC lsp)
  target_link_libraries(newickLsp PUBLIC newickCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(newick_lsp lsp.cpp)
    target_link_libraries(newick_lsp PRIVATE newickLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET newickLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(newick SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(newick)
