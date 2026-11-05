# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID caql
    GRAMMAR    caql/CaqlGrammar.hpp
    NAMESPACE  caql::Caql
    EXTENSIONS .caql
    OUT        caql/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(caqlCore ${_core})
target_link_libraries(caqlCore PUBLIC caqlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(caql_cli cli.cpp)
  target_include_directories(caql_cli PRIVATE cli)
  target_link_libraries(caql_cli PRIVATE caqlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(caqlCli ${_cli})
    target_include_directories(caqlCli PUBLIC cli)
    target_link_libraries(caqlCli PUBLIC caqlCore cursiv::cli)
    target_link_libraries(caql_cli PRIVATE caqlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(caqlLsp ${_lsp})
  target_include_directories(caqlLsp PUBLIC lsp)
  target_link_libraries(caqlLsp PUBLIC caqlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(caql_lsp lsp.cpp)
    target_link_libraries(caql_lsp PRIVATE caqlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET caqlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(caql SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(caql)
