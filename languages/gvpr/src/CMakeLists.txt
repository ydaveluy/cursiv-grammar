# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID gvpr
    GRAMMAR    gvpr/GvprGrammar.hpp
    NAMESPACE  gvpr::Gvpr
    EXTENSIONS .gvpr
    OUT        gvpr/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(gvprCore ${_core})
target_link_libraries(gvprCore PUBLIC gvprGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(gvpr_cli cli.cpp)
  target_include_directories(gvpr_cli PRIVATE cli)
  target_link_libraries(gvpr_cli PRIVATE gvprCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(gvprCli ${_cli})
    target_include_directories(gvprCli PUBLIC cli)
    target_link_libraries(gvprCli PUBLIC gvprCore cursiv::cli)
    target_link_libraries(gvpr_cli PRIVATE gvprCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(gvprLsp ${_lsp})
  target_include_directories(gvprLsp PUBLIC lsp)
  target_link_libraries(gvprLsp PUBLIC gvprCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(gvpr_lsp lsp.cpp)
    target_link_libraries(gvpr_lsp PRIVATE gvprLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET gvprLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(gvpr SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(gvpr)
