# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID istc
    GRAMMAR    istc/IstcGrammar.hpp
    NAMESPACE  istc::Istc
    EXTENSIONS .istc
    OUT        istc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(istcCore ${_core})
target_link_libraries(istcCore PUBLIC istcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(istc_cli cli.cpp)
  target_include_directories(istc_cli PRIVATE cli)
  target_link_libraries(istc_cli PRIVATE istcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(istcCli ${_cli})
    target_include_directories(istcCli PUBLIC cli)
    target_link_libraries(istcCli PUBLIC istcCore cursiv::cli)
    target_link_libraries(istc_cli PRIVATE istcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(istcLsp ${_lsp})
  target_include_directories(istcLsp PUBLIC lsp)
  target_link_libraries(istcLsp PUBLIC istcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(istc_lsp lsp.cpp)
    target_link_libraries(istc_lsp PRIVATE istcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET istcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(istc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(istc)
