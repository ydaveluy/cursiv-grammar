# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID bnf
    GRAMMAR    bnf/BnfGrammar.hpp
    NAMESPACE  bnf::Bnf
    EXTENSIONS .bnf
    OUT        bnf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(bnfCore ${_core})
target_link_libraries(bnfCore PUBLIC bnfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(bnf_cli cli.cpp)
  target_include_directories(bnf_cli PRIVATE cli)
  target_link_libraries(bnf_cli PRIVATE bnfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(bnfCli ${_cli})
    target_include_directories(bnfCli PUBLIC cli)
    target_link_libraries(bnfCli PUBLIC bnfCore cursiv::cli)
    target_link_libraries(bnf_cli PRIVATE bnfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(bnfLsp ${_lsp})
  target_include_directories(bnfLsp PUBLIC lsp)
  target_link_libraries(bnfLsp PUBLIC bnfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(bnf_lsp lsp.cpp)
    target_link_libraries(bnf_lsp PRIVATE bnfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET bnfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(bnf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(bnf)
