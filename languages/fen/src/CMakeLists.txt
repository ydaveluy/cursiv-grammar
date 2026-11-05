# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID fen
    GRAMMAR    fen/FenGrammar.hpp
    NAMESPACE  fen::Fen
    EXTENSIONS .fen
    OUT        fen/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(fenCore ${_core})
target_link_libraries(fenCore PUBLIC fenGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(fen_cli cli.cpp)
  target_include_directories(fen_cli PRIVATE cli)
  target_link_libraries(fen_cli PRIVATE fenCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(fenCli ${_cli})
    target_include_directories(fenCli PUBLIC cli)
    target_link_libraries(fenCli PUBLIC fenCore cursiv::cli)
    target_link_libraries(fen_cli PRIVATE fenCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(fenLsp ${_lsp})
  target_include_directories(fenLsp PUBLIC lsp)
  target_link_libraries(fenLsp PUBLIC fenCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(fen_lsp lsp.cpp)
    target_link_libraries(fen_lsp PRIVATE fenLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET fenLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(fen SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(fen)
