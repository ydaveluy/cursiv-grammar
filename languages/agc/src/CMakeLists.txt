# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID agc
    GRAMMAR    agc/AgcGrammar.hpp
    NAMESPACE  agc::Agc
    EXTENSIONS .agc
    OUT        agc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(agcCore ${_core})
target_link_libraries(agcCore PUBLIC agcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(agc_cli cli.cpp)
  target_include_directories(agc_cli PRIVATE cli)
  target_link_libraries(agc_cli PRIVATE agcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(agcCli ${_cli})
    target_include_directories(agcCli PUBLIC cli)
    target_link_libraries(agcCli PUBLIC agcCore cursiv::cli)
    target_link_libraries(agc_cli PRIVATE agcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(agcLsp ${_lsp})
  target_include_directories(agcLsp PUBLIC lsp)
  target_link_libraries(agcLsp PUBLIC agcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(agc_lsp lsp.cpp)
    target_link_libraries(agc_lsp PRIVATE agcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET agcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(agc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(agc)
