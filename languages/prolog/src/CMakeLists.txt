# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID prolog
    GRAMMAR    prolog/PrologGrammar.hpp
    NAMESPACE  prolog::Prolog
    EXTENSIONS .pl
    OUT        prolog/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(prologCore ${_core})
target_link_libraries(prologCore PUBLIC prologGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(prolog_cli cli.cpp)
  target_include_directories(prolog_cli PRIVATE cli)
  target_link_libraries(prolog_cli PRIVATE prologCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(prologCli ${_cli})
    target_include_directories(prologCli PUBLIC cli)
    target_link_libraries(prologCli PUBLIC prologCore cursiv::cli)
    target_link_libraries(prolog_cli PRIVATE prologCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(prologLsp ${_lsp})
  target_include_directories(prologLsp PUBLIC lsp)
  target_link_libraries(prologLsp PUBLIC prologCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(prolog_lsp lsp.cpp)
    target_link_libraries(prolog_lsp PRIVATE prologLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET prologLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(prolog SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(prolog)
