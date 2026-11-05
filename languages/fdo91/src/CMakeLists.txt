# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID fdo91
    GRAMMAR    fdo91/Fdo91Grammar.hpp
    NAMESPACE  fdo91::Fdo91
    EXTENSIONS .fdo91
    OUT        fdo91/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(fdo91Core ${_core})
target_link_libraries(fdo91Core PUBLIC fdo91Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(fdo91_cli cli.cpp)
  target_include_directories(fdo91_cli PRIVATE cli)
  target_link_libraries(fdo91_cli PRIVATE fdo91Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(fdo91Cli ${_cli})
    target_include_directories(fdo91Cli PUBLIC cli)
    target_link_libraries(fdo91Cli PUBLIC fdo91Core cursiv::cli)
    target_link_libraries(fdo91_cli PRIVATE fdo91Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(fdo91Lsp ${_lsp})
  target_include_directories(fdo91Lsp PUBLIC lsp)
  target_link_libraries(fdo91Lsp PUBLIC fdo91Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(fdo91_lsp lsp.cpp)
    target_link_libraries(fdo91_lsp PRIVATE fdo91Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET fdo91Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(fdo91 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(fdo91)
