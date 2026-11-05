# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID clif
    GRAMMAR    clif/ClifGrammar.hpp
    NAMESPACE  clif::Clif
    EXTENSIONS .clif
    OUT        clif/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(clifCore ${_core})
target_link_libraries(clifCore PUBLIC clifGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(clif_cli cli.cpp)
  target_include_directories(clif_cli PRIVATE cli)
  target_link_libraries(clif_cli PRIVATE clifCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(clifCli ${_cli})
    target_include_directories(clifCli PUBLIC cli)
    target_link_libraries(clifCli PUBLIC clifCore cursiv::cli)
    target_link_libraries(clif_cli PRIVATE clifCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(clifLsp ${_lsp})
  target_include_directories(clifLsp PUBLIC lsp)
  target_link_libraries(clifLsp PUBLIC clifCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(clif_lsp lsp.cpp)
    target_link_libraries(clif_lsp PRIVATE clifLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET clifLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(clif SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(clif)
