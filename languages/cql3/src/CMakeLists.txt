# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID cql3
    GRAMMAR    cql3/Cql3Grammar.hpp
    NAMESPACE  cql3::Cql3
    EXTENSIONS .cql3
    OUT        cql3/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(cql3Core ${_core})
target_link_libraries(cql3Core PUBLIC cql3Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(cql3_cli cli.cpp)
  target_include_directories(cql3_cli PRIVATE cli)
  target_link_libraries(cql3_cli PRIVATE cql3Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(cql3Cli ${_cli})
    target_include_directories(cql3Cli PUBLIC cli)
    target_link_libraries(cql3Cli PUBLIC cql3Core cursiv::cli)
    target_link_libraries(cql3_cli PRIVATE cql3Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(cql3Lsp ${_lsp})
  target_include_directories(cql3Lsp PUBLIC lsp)
  target_link_libraries(cql3Lsp PUBLIC cql3Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(cql3_lsp lsp.cpp)
    target_link_libraries(cql3_lsp PRIVATE cql3Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET cql3Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(cql3 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(cql3)
