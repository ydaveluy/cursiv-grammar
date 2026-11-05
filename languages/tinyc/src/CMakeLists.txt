# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tinyc
    GRAMMAR    tinyc/TinycGrammar.hpp
    NAMESPACE  tinyc::Tinyc
    EXTENSIONS .tinyc
    OUT        tinyc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tinycCore ${_core})
target_link_libraries(tinycCore PUBLIC tinycGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tinyc_cli cli.cpp)
  target_include_directories(tinyc_cli PRIVATE cli)
  target_link_libraries(tinyc_cli PRIVATE tinycCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tinycCli ${_cli})
    target_include_directories(tinycCli PUBLIC cli)
    target_link_libraries(tinycCli PUBLIC tinycCore cursiv::cli)
    target_link_libraries(tinyc_cli PRIVATE tinycCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tinycLsp ${_lsp})
  target_include_directories(tinycLsp PUBLIC lsp)
  target_link_libraries(tinycLsp PUBLIC tinycCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tinyc_lsp lsp.cpp)
    target_link_libraries(tinyc_lsp PRIVATE tinycLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tinycLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tinyc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tinyc)
