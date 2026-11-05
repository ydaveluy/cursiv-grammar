# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID sysmlv2
    GRAMMAR    sysmlv2/SysmlV2Grammar.hpp
    NAMESPACE  sysmlv2::SysmlV2
    EXTENSIONS .sysml
    OUT        sysmlv2/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(sysmlv2Core ${_core})
target_link_libraries(sysmlv2Core PUBLIC sysmlv2Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(sysmlv2_cli cli.cpp)
  target_include_directories(sysmlv2_cli PRIVATE cli)
  target_link_libraries(sysmlv2_cli PRIVATE sysmlv2Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(sysmlv2Cli ${_cli})
    target_include_directories(sysmlv2Cli PUBLIC cli)
    target_link_libraries(sysmlv2Cli PUBLIC sysmlv2Core cursiv::cli)
    target_link_libraries(sysmlv2_cli PRIVATE sysmlv2Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(sysmlv2Lsp ${_lsp})
  target_include_directories(sysmlv2Lsp PUBLIC lsp)
  target_link_libraries(sysmlv2Lsp PUBLIC sysmlv2Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(sysmlv2_lsp lsp.cpp)
    target_link_libraries(sysmlv2_lsp PRIVATE sysmlv2Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET sysmlv2Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(sysmlv2 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(sysmlv2)
