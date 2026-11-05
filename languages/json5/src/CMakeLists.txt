# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID json5
    GRAMMAR    json5/Json5Grammar.hpp
    NAMESPACE  json5::Json5
    EXTENSIONS .json5
    OUT        json5/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(json5Core ${_core})
target_link_libraries(json5Core PUBLIC json5Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(json5_cli cli.cpp)
  target_include_directories(json5_cli PRIVATE cli)
  target_link_libraries(json5_cli PRIVATE json5Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(json5Cli ${_cli})
    target_include_directories(json5Cli PUBLIC cli)
    target_link_libraries(json5Cli PUBLIC json5Core cursiv::cli)
    target_link_libraries(json5_cli PRIVATE json5Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(json5Lsp ${_lsp})
  target_include_directories(json5Lsp PUBLIC lsp)
  target_link_libraries(json5Lsp PUBLIC json5Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(json5_lsp lsp.cpp)
    target_link_libraries(json5_lsp PRIVATE json5Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET json5Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(json5 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(json5)
