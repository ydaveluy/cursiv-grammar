# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID css3
    GRAMMAR    css3/Css3Grammar.hpp
    NAMESPACE  css3::Css3
    EXTENSIONS .css
    OUT        css3/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(css3Core ${_core})
target_link_libraries(css3Core PUBLIC css3Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(css3_cli cli.cpp)
  target_include_directories(css3_cli PRIVATE cli)
  target_link_libraries(css3_cli PRIVATE css3Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(css3Cli ${_cli})
    target_include_directories(css3Cli PUBLIC cli)
    target_link_libraries(css3Cli PUBLIC css3Core cursiv::cli)
    target_link_libraries(css3_cli PRIVATE css3Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(css3Lsp ${_lsp})
  target_include_directories(css3Lsp PUBLIC lsp)
  target_link_libraries(css3Lsp PUBLIC css3Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(css3_lsp lsp.cpp)
    target_link_libraries(css3_lsp PRIVATE css3Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET css3Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(css3 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(css3)
