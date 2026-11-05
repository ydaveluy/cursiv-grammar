# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID ltl
    GRAMMAR    ltl/LtlGrammar.hpp
    NAMESPACE  ltl::Ltl
    EXTENSIONS .ltl
    OUT        ltl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(ltlCore ${_core})
target_link_libraries(ltlCore PUBLIC ltlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ltl_cli cli.cpp)
  target_include_directories(ltl_cli PRIVATE cli)
  target_link_libraries(ltl_cli PRIVATE ltlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(ltlCli ${_cli})
    target_include_directories(ltlCli PUBLIC cli)
    target_link_libraries(ltlCli PUBLIC ltlCore cursiv::cli)
    target_link_libraries(ltl_cli PRIVATE ltlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(ltlLsp ${_lsp})
  target_include_directories(ltlLsp PUBLIC lsp)
  target_link_libraries(ltlLsp PUBLIC ltlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ltl_lsp lsp.cpp)
    target_link_libraries(ltl_lsp PRIVATE ltlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET ltlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ltl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ltl)
