# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID dart2
    GRAMMAR    dart2/Dart2Grammar.hpp
    NAMESPACE  dart2::Dart2
    EXTENSIONS .dart
    OUT        dart2/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(dart2Core ${_core})
target_link_libraries(dart2Core PUBLIC dart2Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(dart2_cli cli.cpp)
  target_include_directories(dart2_cli PRIVATE cli)
  target_link_libraries(dart2_cli PRIVATE dart2Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(dart2Cli ${_cli})
    target_include_directories(dart2Cli PUBLIC cli)
    target_link_libraries(dart2Cli PUBLIC dart2Core cursiv::cli)
    target_link_libraries(dart2_cli PRIVATE dart2Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(dart2Lsp ${_lsp})
  target_include_directories(dart2Lsp PUBLIC lsp)
  target_link_libraries(dart2Lsp PUBLIC dart2Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(dart2_lsp lsp.cpp)
    target_link_libraries(dart2_lsp PRIVATE dart2Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET dart2Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(dart2 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(dart2)
