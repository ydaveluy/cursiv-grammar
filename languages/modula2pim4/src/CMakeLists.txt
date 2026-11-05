# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID modula2pim4
    GRAMMAR    modula2pim4/Modula2pim4Grammar.hpp
    NAMESPACE  modula2pim4::Modula2pim4
    EXTENSIONS .mod
    OUT        modula2pim4/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(modula2pim4Core ${_core})
target_link_libraries(modula2pim4Core PUBLIC modula2pim4Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(modula2pim4_cli cli.cpp)
  target_include_directories(modula2pim4_cli PRIVATE cli)
  target_link_libraries(modula2pim4_cli PRIVATE modula2pim4Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(modula2pim4Cli ${_cli})
    target_include_directories(modula2pim4Cli PUBLIC cli)
    target_link_libraries(modula2pim4Cli PUBLIC modula2pim4Core cursiv::cli)
    target_link_libraries(modula2pim4_cli PRIVATE modula2pim4Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(modula2pim4Lsp ${_lsp})
  target_include_directories(modula2pim4Lsp PUBLIC lsp)
  target_link_libraries(modula2pim4Lsp PUBLIC modula2pim4Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(modula2pim4_lsp lsp.cpp)
    target_link_libraries(modula2pim4_lsp PRIVATE modula2pim4Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET modula2pim4Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(modula2pim4 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(modula2pim4)
