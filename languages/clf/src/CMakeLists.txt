# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID clf
    GRAMMAR    clf/ClfGrammar.hpp
    NAMESPACE  clf::Clf
    EXTENSIONS .clf
    OUT        clf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(clfCore ${_core})
target_link_libraries(clfCore PUBLIC clfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(clf_cli cli.cpp)
  target_include_directories(clf_cli PRIVATE cli)
  target_link_libraries(clf_cli PRIVATE clfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(clfCli ${_cli})
    target_include_directories(clfCli PUBLIC cli)
    target_link_libraries(clfCli PUBLIC clfCore cursiv::cli)
    target_link_libraries(clf_cli PRIVATE clfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(clfLsp ${_lsp})
  target_include_directories(clfLsp PUBLIC lsp)
  target_link_libraries(clfLsp PUBLIC clfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(clf_lsp lsp.cpp)
    target_link_libraries(clf_lsp PRIVATE clfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET clfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(clf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(clf)
