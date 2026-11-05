# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tnt
    GRAMMAR    tnt/TntGrammar.hpp
    NAMESPACE  tnt::Tnt
    EXTENSIONS .tnt
    OUT        tnt/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tntCore ${_core})
target_link_libraries(tntCore PUBLIC tntGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tnt_cli cli.cpp)
  target_include_directories(tnt_cli PRIVATE cli)
  target_link_libraries(tnt_cli PRIVATE tntCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tntCli ${_cli})
    target_include_directories(tntCli PUBLIC cli)
    target_link_libraries(tntCli PUBLIC tntCore cursiv::cli)
    target_link_libraries(tnt_cli PRIVATE tntCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tntLsp ${_lsp})
  target_include_directories(tntLsp PUBLIC lsp)
  target_link_libraries(tntLsp PUBLIC tntCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tnt_lsp lsp.cpp)
    target_link_libraries(tnt_lsp PRIVATE tntLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tntLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tnt SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tnt)
