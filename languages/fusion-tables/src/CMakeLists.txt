# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID fusiontables
    GRAMMAR    fusiontables/FusionTablesGrammar.hpp
    NAMESPACE  fusiontables::FusionTables
    EXTENSIONS .fql
    OUT        fusiontables/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(fusiontablesCore ${_core})
target_link_libraries(fusiontablesCore PUBLIC fusiontablesGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(fusiontables_cli cli.cpp)
  target_include_directories(fusiontables_cli PRIVATE cli)
  target_link_libraries(fusiontables_cli PRIVATE fusiontablesCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(fusiontablesCli ${_cli})
    target_include_directories(fusiontablesCli PUBLIC cli)
    target_link_libraries(fusiontablesCli PUBLIC fusiontablesCore cursiv::cli)
    target_link_libraries(fusiontables_cli PRIVATE fusiontablesCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(fusiontablesLsp ${_lsp})
  target_include_directories(fusiontablesLsp PUBLIC lsp)
  target_link_libraries(fusiontablesLsp PUBLIC fusiontablesCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(fusiontables_lsp lsp.cpp)
    target_link_libraries(fusiontables_lsp PRIVATE fusiontablesLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET fusiontablesLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(fusiontables SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(fusiontables)
