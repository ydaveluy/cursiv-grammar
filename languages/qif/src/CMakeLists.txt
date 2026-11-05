# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID qif
    GRAMMAR    qif/QifGrammar.hpp
    NAMESPACE  qif::Qif
    EXTENSIONS .qif
    OUT        qif/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(qifCore ${_core})
target_link_libraries(qifCore PUBLIC qifGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(qif_cli cli.cpp)
  target_include_directories(qif_cli PRIVATE cli)
  target_link_libraries(qif_cli PRIVATE qifCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(qifCli ${_cli})
    target_include_directories(qifCli PUBLIC cli)
    target_link_libraries(qifCli PUBLIC qifCore cursiv::cli)
    target_link_libraries(qif_cli PRIVATE qifCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(qifLsp ${_lsp})
  target_include_directories(qifLsp PUBLIC lsp)
  target_link_libraries(qifLsp PUBLIC qifCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(qif_lsp lsp.cpp)
    target_link_libraries(qif_lsp PRIVATE qifLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET qifLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(qif SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(qif)
