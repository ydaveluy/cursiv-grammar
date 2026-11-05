# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID muparser
    GRAMMAR    muparser/MuparserGrammar.hpp
    NAMESPACE  muparser::Muparser
    EXTENSIONS .muparser
    OUT        muparser/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(muparserCore ${_core})
target_link_libraries(muparserCore PUBLIC muparserGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(muparser_cli cli.cpp)
  target_include_directories(muparser_cli PRIVATE cli)
  target_link_libraries(muparser_cli PRIVATE muparserCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(muparserCli ${_cli})
    target_include_directories(muparserCli PUBLIC cli)
    target_link_libraries(muparserCli PUBLIC muparserCore cursiv::cli)
    target_link_libraries(muparser_cli PRIVATE muparserCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(muparserLsp ${_lsp})
  target_include_directories(muparserLsp PUBLIC lsp)
  target_link_libraries(muparserLsp PUBLIC muparserCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(muparser_lsp lsp.cpp)
    target_link_libraries(muparser_lsp PRIVATE muparserLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET muparserLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(muparser SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(muparser)
