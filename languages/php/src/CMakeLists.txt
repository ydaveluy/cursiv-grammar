# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID php
    GRAMMAR    php/PhpGrammar.hpp
    NAMESPACE  php::Php
    EXTENSIONS .php
    OUT        php/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(phpCore ${_core})
target_link_libraries(phpCore PUBLIC phpGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(php_cli cli.cpp)
  target_include_directories(php_cli PRIVATE cli)
  target_link_libraries(php_cli PRIVATE phpCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(phpCli ${_cli})
    target_include_directories(phpCli PUBLIC cli)
    target_link_libraries(phpCli PUBLIC phpCore cursiv::cli)
    target_link_libraries(php_cli PRIVATE phpCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(phpLsp ${_lsp})
  target_include_directories(phpLsp PUBLIC lsp)
  target_link_libraries(phpLsp PUBLIC phpCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(php_lsp lsp.cpp)
    target_link_libraries(php_lsp PRIVATE phpLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET phpLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(php SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(php)
