# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID kquery
    GRAMMAR    kquery/KqueryGrammar.hpp
    NAMESPACE  kquery::Kquery
    EXTENSIONS .kquery
    OUT        kquery/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(kqueryCore ${_core})
target_link_libraries(kqueryCore PUBLIC kqueryGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(kquery_cli cli.cpp)
  target_include_directories(kquery_cli PRIVATE cli)
  target_link_libraries(kquery_cli PRIVATE kqueryCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(kqueryCli ${_cli})
    target_include_directories(kqueryCli PUBLIC cli)
    target_link_libraries(kqueryCli PUBLIC kqueryCore cursiv::cli)
    target_link_libraries(kquery_cli PRIVATE kqueryCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(kqueryLsp ${_lsp})
  target_include_directories(kqueryLsp PUBLIC lsp)
  target_link_libraries(kqueryLsp PUBLIC kqueryCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(kquery_lsp lsp.cpp)
    target_link_libraries(kquery_lsp PRIVATE kqueryLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET kqueryLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(kquery SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(kquery)
