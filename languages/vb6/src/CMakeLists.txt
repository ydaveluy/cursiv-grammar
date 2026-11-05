# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID vb6
    GRAMMAR    vb6/Vb6Grammar.hpp
    NAMESPACE  vb6::Vb6
    EXTENSIONS .bas .cls .frm
    OUT        vb6/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(vb6Core ${_core})
target_link_libraries(vb6Core PUBLIC vb6Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(vb6_cli cli.cpp)
  target_include_directories(vb6_cli PRIVATE cli)
  target_link_libraries(vb6_cli PRIVATE vb6Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(vb6Cli ${_cli})
    target_include_directories(vb6Cli PUBLIC cli)
    target_link_libraries(vb6Cli PUBLIC vb6Core cursiv::cli)
    target_link_libraries(vb6_cli PRIVATE vb6Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(vb6Lsp ${_lsp})
  target_include_directories(vb6Lsp PUBLIC lsp)
  target_link_libraries(vb6Lsp PUBLIC vb6Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(vb6_lsp lsp.cpp)
    target_link_libraries(vb6_lsp PRIVATE vb6Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET vb6Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(vb6 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(vb6)
