# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID suokif
    GRAMMAR    suokif/SuokifGrammar.hpp
    NAMESPACE  suokif::Suokif
    EXTENSIONS .kif
    OUT        suokif/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(suokifCore ${_core})
target_link_libraries(suokifCore PUBLIC suokifGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(suokif_cli cli.cpp)
  target_include_directories(suokif_cli PRIVATE cli)
  target_link_libraries(suokif_cli PRIVATE suokifCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(suokifCli ${_cli})
    target_include_directories(suokifCli PUBLIC cli)
    target_link_libraries(suokifCli PUBLIC suokifCore cursiv::cli)
    target_link_libraries(suokif_cli PRIVATE suokifCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(suokifLsp ${_lsp})
  target_include_directories(suokifLsp PUBLIC lsp)
  target_link_libraries(suokifLsp PUBLIC suokifCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(suokif_lsp lsp.cpp)
    target_link_libraries(suokif_lsp PRIVATE suokifLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET suokifLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(suokif SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(suokif)
