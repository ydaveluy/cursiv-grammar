# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID ttm
    GRAMMAR    ttm/TtmGrammar.hpp
    NAMESPACE  ttm::Ttm
    EXTENSIONS .ttm
    OUT        ttm/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(ttmCore ${_core})
target_link_libraries(ttmCore PUBLIC ttmGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ttm_cli cli.cpp)
  target_include_directories(ttm_cli PRIVATE cli)
  target_link_libraries(ttm_cli PRIVATE ttmCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(ttmCli ${_cli})
    target_include_directories(ttmCli PUBLIC cli)
    target_link_libraries(ttmCli PUBLIC ttmCore cursiv::cli)
    target_link_libraries(ttm_cli PRIVATE ttmCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(ttmLsp ${_lsp})
  target_include_directories(ttmLsp PUBLIC lsp)
  target_link_libraries(ttmLsp PUBLIC ttmCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ttm_lsp lsp.cpp)
    target_link_libraries(ttm_lsp PRIVATE ttmLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET ttmLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ttm SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ttm)
