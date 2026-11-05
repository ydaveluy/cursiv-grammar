# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID gql
    GRAMMAR    gql/GqlGrammar.hpp
    NAMESPACE  gql::Gql
    EXTENSIONS .gql
    OUT        gql/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(gqlCore ${_core})
target_link_libraries(gqlCore PUBLIC gqlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(gql_cli cli.cpp)
  target_include_directories(gql_cli PRIVATE cli)
  target_link_libraries(gql_cli PRIVATE gqlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(gqlCli ${_cli})
    target_include_directories(gqlCli PUBLIC cli)
    target_link_libraries(gqlCli PUBLIC gqlCore cursiv::cli)
    target_link_libraries(gql_cli PRIVATE gqlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(gqlLsp ${_lsp})
  target_include_directories(gqlLsp PUBLIC lsp)
  target_link_libraries(gqlLsp PUBLIC gqlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(gql_lsp lsp.cpp)
    target_link_libraries(gql_lsp PRIVATE gqlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET gqlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(gql SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(gql)
