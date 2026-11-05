# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rfc1960
    GRAMMAR    rfc1960/Rfc1960Grammar.hpp
    NAMESPACE  rfc1960::Rfc1960
    EXTENSIONS .rfc1960
    OUT        rfc1960/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(rfc1960Core ${_core})
target_link_libraries(rfc1960Core PUBLIC rfc1960Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rfc1960_cli cli.cpp)
  target_include_directories(rfc1960_cli PRIVATE cli)
  target_link_libraries(rfc1960_cli PRIVATE rfc1960Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(rfc1960Cli ${_cli})
    target_include_directories(rfc1960Cli PUBLIC cli)
    target_link_libraries(rfc1960Cli PUBLIC rfc1960Core cursiv::cli)
    target_link_libraries(rfc1960_cli PRIVATE rfc1960Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(rfc1960Lsp ${_lsp})
  target_include_directories(rfc1960Lsp PUBLIC lsp)
  target_link_libraries(rfc1960Lsp PUBLIC rfc1960Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rfc1960_lsp lsp.cpp)
    target_link_libraries(rfc1960_lsp PRIVATE rfc1960Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET rfc1960Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rfc1960 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rfc1960)
