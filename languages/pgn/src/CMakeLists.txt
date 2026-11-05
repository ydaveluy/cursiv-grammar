# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID pgn
    GRAMMAR    pgn/PgnGrammar.hpp
    NAMESPACE  pgn::Pgn
    EXTENSIONS .pgn
    OUT        pgn/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(pgnCore ${_core})
target_link_libraries(pgnCore PUBLIC pgnGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(pgn_cli cli.cpp)
  target_include_directories(pgn_cli PRIVATE cli)
  target_link_libraries(pgn_cli PRIVATE pgnCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(pgnCli ${_cli})
    target_include_directories(pgnCli PUBLIC cli)
    target_link_libraries(pgnCli PUBLIC pgnCore cursiv::cli)
    target_link_libraries(pgn_cli PRIVATE pgnCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(pgnLsp ${_lsp})
  target_include_directories(pgnLsp PUBLIC lsp)
  target_link_libraries(pgnLsp PUBLIC pgnCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(pgn_lsp lsp.cpp)
    target_link_libraries(pgn_lsp PRIVATE pgnLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET pgnLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(pgn SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(pgn)
