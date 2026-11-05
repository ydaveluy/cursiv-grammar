# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tsv
    GRAMMAR    tsv/TsvGrammar.hpp
    NAMESPACE  tsv::Tsv
    EXTENSIONS .tsv
    OUT        tsv/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tsvCore ${_core})
target_link_libraries(tsvCore PUBLIC tsvGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tsv_cli cli.cpp)
  target_include_directories(tsv_cli PRIVATE cli)
  target_link_libraries(tsv_cli PRIVATE tsvCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tsvCli ${_cli})
    target_include_directories(tsvCli PUBLIC cli)
    target_link_libraries(tsvCli PUBLIC tsvCore cursiv::cli)
    target_link_libraries(tsv_cli PRIVATE tsvCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tsvLsp ${_lsp})
  target_include_directories(tsvLsp PUBLIC lsp)
  target_link_libraries(tsvLsp PUBLIC tsvCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tsv_lsp lsp.cpp)
    target_link_libraries(tsv_lsp PRIVATE tsvLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tsvLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tsv SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tsv)
