# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tinymud
    GRAMMAR    tinymud/TinymudGrammar.hpp
    NAMESPACE  tinymud::Tinymud
    EXTENSIONS .tinymud
    OUT        tinymud/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tinymudCore ${_core})
target_link_libraries(tinymudCore PUBLIC tinymudGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tinymud_cli cli.cpp)
  target_include_directories(tinymud_cli PRIVATE cli)
  target_link_libraries(tinymud_cli PRIVATE tinymudCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tinymudCli ${_cli})
    target_include_directories(tinymudCli PUBLIC cli)
    target_link_libraries(tinymudCli PUBLIC tinymudCore cursiv::cli)
    target_link_libraries(tinymud_cli PRIVATE tinymudCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tinymudLsp ${_lsp})
  target_include_directories(tinymudLsp PUBLIC lsp)
  target_link_libraries(tinymudLsp PUBLIC tinymudCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tinymud_lsp lsp.cpp)
    target_link_libraries(tinymud_lsp PRIVATE tinymudLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tinymudLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tinymud SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tinymud)
