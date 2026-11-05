# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID pl0
    GRAMMAR    pl0/Pl0Grammar.hpp
    NAMESPACE  pl0::Pl0
    EXTENSIONS .pl0
    OUT        pl0/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(pl0Core ${_core})
target_link_libraries(pl0Core PUBLIC pl0Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(pl0_cli cli.cpp)
  target_include_directories(pl0_cli PRIVATE cli)
  target_link_libraries(pl0_cli PRIVATE pl0Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(pl0Cli ${_cli})
    target_include_directories(pl0Cli PUBLIC cli)
    target_link_libraries(pl0Cli PUBLIC pl0Core cursiv::cli)
    target_link_libraries(pl0_cli PRIVATE pl0Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(pl0Lsp ${_lsp})
  target_include_directories(pl0Lsp PUBLIC lsp)
  target_link_libraries(pl0Lsp PUBLIC pl0Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(pl0_lsp lsp.cpp)
    target_link_libraries(pl0_lsp PRIVATE pl0Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET pl0Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(pl0 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(pl0)
