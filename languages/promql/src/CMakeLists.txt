# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID promql
    GRAMMAR    promql/PromqlGrammar.hpp
    NAMESPACE  promql::Promql
    EXTENSIONS .promql
    OUT        promql/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(promqlCore ${_core})
target_link_libraries(promqlCore PUBLIC promqlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(promql_cli cli.cpp)
  target_include_directories(promql_cli PRIVATE cli)
  target_link_libraries(promql_cli PRIVATE promqlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(promqlCli ${_cli})
    target_include_directories(promqlCli PUBLIC cli)
    target_link_libraries(promqlCli PUBLIC promqlCore cursiv::cli)
    target_link_libraries(promql_cli PRIVATE promqlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(promqlLsp ${_lsp})
  target_include_directories(promqlLsp PUBLIC lsp)
  target_link_libraries(promqlLsp PUBLIC promqlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(promql_lsp lsp.cpp)
    target_link_libraries(promql_lsp PRIVATE promqlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET promqlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(promql SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(promql)
