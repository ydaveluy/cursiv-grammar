# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID d2
    GRAMMAR    d2/D2Grammar.hpp
    NAMESPACE  d2::D2
    EXTENSIONS .d2
    OUT        d2/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(d2Core ${_core})
target_link_libraries(d2Core PUBLIC d2Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(d2_cli cli.cpp)
  target_include_directories(d2_cli PRIVATE cli)
  target_link_libraries(d2_cli PRIVATE d2Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(d2Cli ${_cli})
    target_include_directories(d2Cli PUBLIC cli)
    target_link_libraries(d2Cli PUBLIC d2Core cursiv::cli)
    target_link_libraries(d2_cli PRIVATE d2Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(d2Lsp ${_lsp})
  target_include_directories(d2Lsp PUBLIC lsp)
  target_link_libraries(d2Lsp PUBLIC d2Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(d2_lsp lsp.cpp)
    target_link_libraries(d2_lsp PRIVATE d2Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET d2Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(d2 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(d2)
