# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID graphemes
    GRAMMAR    graphemes/GraphemesGrammar.hpp
    NAMESPACE  graphemes::Graphemes
    EXTENSIONS .graphemes
    OUT        graphemes/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(graphemesCore ${_core})
target_link_libraries(graphemesCore PUBLIC graphemesGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(graphemes_cli cli.cpp)
  target_include_directories(graphemes_cli PRIVATE cli)
  target_link_libraries(graphemes_cli PRIVATE graphemesCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(graphemesCli ${_cli})
    target_include_directories(graphemesCli PUBLIC cli)
    target_link_libraries(graphemesCli PUBLIC graphemesCore cursiv::cli)
    target_link_libraries(graphemes_cli PRIVATE graphemesCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(graphemesLsp ${_lsp})
  target_include_directories(graphemesLsp PUBLIC lsp)
  target_link_libraries(graphemesLsp PUBLIC graphemesCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(graphemes_lsp lsp.cpp)
    target_link_libraries(graphemes_lsp PRIVATE graphemesLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET graphemesLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(graphemes SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(graphemes)
