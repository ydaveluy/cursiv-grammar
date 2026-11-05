# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID scss
    GRAMMAR    scss/ScssGrammar.hpp
    NAMESPACE  scss::Scss
    EXTENSIONS .scss
    OUT        scss/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(scssCore ${_core})
target_link_libraries(scssCore PUBLIC scssGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(scss_cli cli.cpp)
  target_include_directories(scss_cli PRIVATE cli)
  target_link_libraries(scss_cli PRIVATE scssCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(scssCli ${_cli})
    target_include_directories(scssCli PUBLIC cli)
    target_link_libraries(scssCli PUBLIC scssCore cursiv::cli)
    target_link_libraries(scss_cli PRIVATE scssCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(scssLsp ${_lsp})
  target_include_directories(scssLsp PUBLIC lsp)
  target_link_libraries(scssLsp PUBLIC scssCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(scss_lsp lsp.cpp)
    target_link_libraries(scss_lsp PRIVATE scssLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET scssLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(scss SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(scss)
