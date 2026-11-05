# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID quakemap
    GRAMMAR    quakemap/QuakemapGrammar.hpp
    NAMESPACE  quakemap::Quakemap
    EXTENSIONS .map
    OUT        quakemap/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(quakemapCore ${_core})
target_link_libraries(quakemapCore PUBLIC quakemapGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(quakemap_cli cli.cpp)
  target_include_directories(quakemap_cli PRIVATE cli)
  target_link_libraries(quakemap_cli PRIVATE quakemapCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(quakemapCli ${_cli})
    target_include_directories(quakemapCli PUBLIC cli)
    target_link_libraries(quakemapCli PUBLIC quakemapCore cursiv::cli)
    target_link_libraries(quakemap_cli PRIVATE quakemapCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(quakemapLsp ${_lsp})
  target_include_directories(quakemapLsp PUBLIC lsp)
  target_link_libraries(quakemapLsp PUBLIC quakemapCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(quakemap_lsp lsp.cpp)
    target_link_libraries(quakemap_lsp PRIVATE quakemapLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET quakemapLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(quakemap SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(quakemap)
