# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tiny
    GRAMMAR    tiny/TinyGrammar.hpp
    NAMESPACE  tiny::Tiny
    EXTENSIONS .tiny
    OUT        tiny/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tinyCore ${_core})
target_link_libraries(tinyCore PUBLIC tinyGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tiny_cli cli.cpp)
  target_include_directories(tiny_cli PRIVATE cli)
  target_link_libraries(tiny_cli PRIVATE tinyCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tinyCli ${_cli})
    target_include_directories(tinyCli PUBLIC cli)
    target_link_libraries(tinyCli PUBLIC tinyCore cursiv::cli)
    target_link_libraries(tiny_cli PRIVATE tinyCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tinyLsp ${_lsp})
  target_include_directories(tinyLsp PUBLIC lsp)
  target_link_libraries(tinyLsp PUBLIC tinyCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tiny_lsp lsp.cpp)
    target_link_libraries(tiny_lsp PRIVATE tinyLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tinyLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tiny SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tiny)
