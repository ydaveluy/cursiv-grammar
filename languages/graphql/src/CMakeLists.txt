# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID graphql
    GRAMMAR    graphql/GraphqlGrammar.hpp
    NAMESPACE  graphql::Graphql
    EXTENSIONS .graphql
    OUT        graphql/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(graphqlCore ${_core})
target_link_libraries(graphqlCore PUBLIC graphqlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(graphql_cli cli.cpp)
  target_include_directories(graphql_cli PRIVATE cli)
  target_link_libraries(graphql_cli PRIVATE graphqlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(graphqlCli ${_cli})
    target_include_directories(graphqlCli PUBLIC cli)
    target_link_libraries(graphqlCli PUBLIC graphqlCore cursiv::cli)
    target_link_libraries(graphql_cli PRIVATE graphqlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(graphqlLsp ${_lsp})
  target_include_directories(graphqlLsp PUBLIC lsp)
  target_link_libraries(graphqlLsp PUBLIC graphqlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(graphql_lsp lsp.cpp)
    target_link_libraries(graphql_lsp PRIVATE graphqlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET graphqlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(graphql SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(graphql)
