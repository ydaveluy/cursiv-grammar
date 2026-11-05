# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID gff3
    GRAMMAR    gff3/Gff3Grammar.hpp
    NAMESPACE  gff3::Gff3
    EXTENSIONS .gff
    OUT        gff3/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(gff3Core ${_core})
target_link_libraries(gff3Core PUBLIC gff3Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(gff3_cli cli.cpp)
  target_include_directories(gff3_cli PRIVATE cli)
  target_link_libraries(gff3_cli PRIVATE gff3Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(gff3Cli ${_cli})
    target_include_directories(gff3Cli PUBLIC cli)
    target_link_libraries(gff3Cli PUBLIC gff3Core cursiv::cli)
    target_link_libraries(gff3_cli PRIVATE gff3Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(gff3Lsp ${_lsp})
  target_include_directories(gff3Lsp PUBLIC lsp)
  target_link_libraries(gff3Lsp PUBLIC gff3Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(gff3_lsp lsp.cpp)
    target_link_libraries(gff3_lsp PRIVATE gff3Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET gff3Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(gff3 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(gff3)
