# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID edif300
    GRAMMAR    edif300/Edif300Grammar.hpp
    NAMESPACE  edif300::Edif300
    EXTENSIONS .edf
    OUT        edif300/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(edif300Core ${_core})
target_link_libraries(edif300Core PUBLIC edif300Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(edif300_cli cli.cpp)
  target_include_directories(edif300_cli PRIVATE cli)
  target_link_libraries(edif300_cli PRIVATE edif300Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(edif300Cli ${_cli})
    target_include_directories(edif300Cli PUBLIC cli)
    target_link_libraries(edif300Cli PUBLIC edif300Core cursiv::cli)
    target_link_libraries(edif300_cli PRIVATE edif300Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(edif300Lsp ${_lsp})
  target_include_directories(edif300Lsp PUBLIC lsp)
  target_link_libraries(edif300Lsp PUBLIC edif300Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(edif300_lsp lsp.cpp)
    target_link_libraries(edif300_lsp PRIVATE edif300Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET edif300Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(edif300 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(edif300)
