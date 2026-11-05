# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID teal
    GRAMMAR    teal/TealGrammar.hpp
    NAMESPACE  teal::Teal
    EXTENSIONS .tl
    OUT        teal/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tealCore ${_core})
target_link_libraries(tealCore PUBLIC tealGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(teal_cli cli.cpp)
  target_include_directories(teal_cli PRIVATE cli)
  target_link_libraries(teal_cli PRIVATE tealCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tealCli ${_cli})
    target_include_directories(tealCli PUBLIC cli)
    target_link_libraries(tealCli PUBLIC tealCore cursiv::cli)
    target_link_libraries(teal_cli PRIVATE tealCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tealLsp ${_lsp})
  target_include_directories(tealLsp PUBLIC lsp)
  target_link_libraries(tealLsp PUBLIC tealCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(teal_lsp lsp.cpp)
    target_link_libraries(teal_lsp PRIVATE tealLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tealLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(teal SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(teal)
