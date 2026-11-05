# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID wktcrsv1
    GRAMMAR    wktcrsv1/WktCrsV1Grammar.hpp
    NAMESPACE  wktcrsv1::WktCrsV1
    EXTENSIONS .wktcrsv1
    OUT        wktcrsv1/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(wktcrsv1Core ${_core})
target_link_libraries(wktcrsv1Core PUBLIC wktcrsv1Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(wktcrsv1_cli cli.cpp)
  target_include_directories(wktcrsv1_cli PRIVATE cli)
  target_link_libraries(wktcrsv1_cli PRIVATE wktcrsv1Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(wktcrsv1Cli ${_cli})
    target_include_directories(wktcrsv1Cli PUBLIC cli)
    target_link_libraries(wktcrsv1Cli PUBLIC wktcrsv1Core cursiv::cli)
    target_link_libraries(wktcrsv1_cli PRIVATE wktcrsv1Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(wktcrsv1Lsp ${_lsp})
  target_include_directories(wktcrsv1Lsp PUBLIC lsp)
  target_link_libraries(wktcrsv1Lsp PUBLIC wktcrsv1Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(wktcrsv1_lsp lsp.cpp)
    target_link_libraries(wktcrsv1_lsp PRIVATE wktcrsv1Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET wktcrsv1Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(wktcrsv1 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(wktcrsv1)
