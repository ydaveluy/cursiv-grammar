# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
# The seven PowerScript source-export kinds this grammar reads (window,
# user object, menu, application, function, structure, query); .srd
# (DataWindow) is a different grammar, ported separately as powerbuilderdw.
cursiv_generate_language(ID powerbuilder
    GRAMMAR    powerbuilder/PowerbuilderGrammar.hpp
    NAMESPACE  powerbuilder::Powerbuilder
    EXTENSIONS .srw .sru .srm .sra .srf .srs .srq
    OUT        powerbuilder/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(powerbuilderCore ${_core})
target_link_libraries(powerbuilderCore PUBLIC powerbuilderGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(powerbuilder_cli cli.cpp)
  target_include_directories(powerbuilder_cli PRIVATE cli)
  target_link_libraries(powerbuilder_cli PRIVATE powerbuilderCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(powerbuilderCli ${_cli})
    target_include_directories(powerbuilderCli PUBLIC cli)
    target_link_libraries(powerbuilderCli PUBLIC powerbuilderCore cursiv::cli)
    target_link_libraries(powerbuilder_cli PRIVATE powerbuilderCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(powerbuilderLsp ${_lsp})
  target_include_directories(powerbuilderLsp PUBLIC lsp)
  target_link_libraries(powerbuilderLsp PUBLIC powerbuilderCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(powerbuilder_lsp lsp.cpp)
    target_link_libraries(powerbuilder_lsp PRIVATE powerbuilderLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET powerbuilderLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(powerbuilder SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(powerbuilder)
