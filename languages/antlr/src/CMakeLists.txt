# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID antlr
    GRAMMAR    antlr/AntlrGrammar.hpp
    NAMESPACE  antlr::Antlr
    EXTENSIONS .g4
    OUT        antlr/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(antlrCore ${_core})
target_link_libraries(antlrCore PUBLIC antlrGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(antlr_cli cli.cpp)
  target_include_directories(antlr_cli PRIVATE cli)
  target_link_libraries(antlr_cli PRIVATE antlrCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(antlrCli ${_cli})
    target_include_directories(antlrCli PUBLIC cli)
    target_link_libraries(antlrCli PUBLIC antlrCore cursiv::cli)
    target_link_libraries(antlr_cli PRIVATE antlrCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(antlrLsp ${_lsp})
  target_include_directories(antlrLsp PUBLIC lsp)
  target_link_libraries(antlrLsp PUBLIC antlrCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(antlr_lsp lsp.cpp)
    target_link_libraries(antlr_lsp PRIVATE antlrLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET antlrLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(antlr SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(antlr)
