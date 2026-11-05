# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rfc3080
    GRAMMAR    rfc3080/Rfc3080Grammar.hpp
    NAMESPACE  rfc3080::Rfc3080
    EXTENSIONS .beep
    OUT        rfc3080/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(rfc3080Core ${_core})
target_link_libraries(rfc3080Core PUBLIC rfc3080Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rfc3080_cli cli.cpp)
  target_include_directories(rfc3080_cli PRIVATE cli)
  target_link_libraries(rfc3080_cli PRIVATE rfc3080Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(rfc3080Cli ${_cli})
    target_include_directories(rfc3080Cli PUBLIC cli)
    target_link_libraries(rfc3080Cli PUBLIC rfc3080Core cursiv::cli)
    target_link_libraries(rfc3080_cli PRIVATE rfc3080Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(rfc3080Lsp ${_lsp})
  target_include_directories(rfc3080Lsp PUBLIC lsp)
  target_link_libraries(rfc3080Lsp PUBLIC rfc3080Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rfc3080_lsp lsp.cpp)
    target_link_libraries(rfc3080_lsp PRIVATE rfc3080Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET rfc3080Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rfc3080 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rfc3080)
