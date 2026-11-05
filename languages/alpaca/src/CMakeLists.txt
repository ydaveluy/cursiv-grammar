# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID alpaca
    GRAMMAR    alpaca/AlpacaGrammar.hpp
    NAMESPACE  alpaca::Alpaca
    EXTENSIONS .alp
    OUT        alpaca/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(alpacaCore ${_core})
target_link_libraries(alpacaCore PUBLIC alpacaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(alpaca_cli cli.cpp)
  target_include_directories(alpaca_cli PRIVATE cli)
  target_link_libraries(alpaca_cli PRIVATE alpacaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(alpacaCli ${_cli})
    target_include_directories(alpacaCli PUBLIC cli)
    target_link_libraries(alpacaCli PUBLIC alpacaCore cursiv::cli)
    target_link_libraries(alpaca_cli PRIVATE alpacaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(alpacaLsp ${_lsp})
  target_include_directories(alpacaLsp PUBLIC lsp)
  target_link_libraries(alpacaLsp PUBLIC alpacaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(alpaca_lsp lsp.cpp)
    target_link_libraries(alpaca_lsp PRIVATE alpacaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET alpacaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(alpaca SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(alpaca)
