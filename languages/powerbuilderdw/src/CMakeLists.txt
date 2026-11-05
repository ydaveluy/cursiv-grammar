# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID powerbuilderdw
    GRAMMAR    powerbuilderdw/PowerbuilderdwGrammar.hpp
    NAMESPACE  powerbuilderdw::Powerbuilderdw
    EXTENSIONS .srd
    OUT        powerbuilderdw/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(powerbuilderdwCore ${_core})
target_link_libraries(powerbuilderdwCore PUBLIC powerbuilderdwGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(powerbuilderdw_cli cli.cpp)
  target_include_directories(powerbuilderdw_cli PRIVATE cli)
  target_link_libraries(powerbuilderdw_cli PRIVATE powerbuilderdwCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(powerbuilderdwCli ${_cli})
    target_include_directories(powerbuilderdwCli PUBLIC cli)
    target_link_libraries(powerbuilderdwCli PUBLIC powerbuilderdwCore cursiv::cli)
    target_link_libraries(powerbuilderdw_cli PRIVATE powerbuilderdwCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(powerbuilderdwLsp ${_lsp})
  target_include_directories(powerbuilderdwLsp PUBLIC lsp)
  target_link_libraries(powerbuilderdwLsp PUBLIC powerbuilderdwCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(powerbuilderdw_lsp lsp.cpp)
    target_link_libraries(powerbuilderdw_lsp PRIVATE powerbuilderdwLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET powerbuilderdwLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(powerbuilderdw SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(powerbuilderdw)
