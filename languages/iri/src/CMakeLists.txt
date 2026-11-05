# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID iri
    GRAMMAR    iri/IriGrammar.hpp
    NAMESPACE  iri::Iri
    EXTENSIONS .iri
    OUT        iri/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(iriCore ${_core})
target_link_libraries(iriCore PUBLIC iriGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(iri_cli cli.cpp)
  target_include_directories(iri_cli PRIVATE cli)
  target_link_libraries(iri_cli PRIVATE iriCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(iriCli ${_cli})
    target_include_directories(iriCli PUBLIC cli)
    target_link_libraries(iriCli PUBLIC iriCore cursiv::cli)
    target_link_libraries(iri_cli PRIVATE iriCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(iriLsp ${_lsp})
  target_include_directories(iriLsp PUBLIC lsp)
  target_link_libraries(iriLsp PUBLIC iriCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(iri_lsp lsp.cpp)
    target_link_libraries(iri_lsp PRIVATE iriLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET iriLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(iri SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(iri)
