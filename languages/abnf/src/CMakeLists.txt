# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID abnf
    GRAMMAR    abnf/AbnfGrammar.hpp
    NAMESPACE  abnf::Abnf
    EXTENSIONS .abnf
    OUT        abnf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(abnfCore ${_core})
target_link_libraries(abnfCore PUBLIC abnfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(abnf_cli cli.cpp)
  target_include_directories(abnf_cli PRIVATE cli)
  target_link_libraries(abnf_cli PRIVATE abnfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(abnfCli ${_cli})
    target_include_directories(abnfCli PUBLIC cli)
    target_link_libraries(abnfCli PUBLIC abnfCore cursiv::cli)
    target_link_libraries(abnf_cli PRIVATE abnfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(abnfLsp ${_lsp})
  target_include_directories(abnfLsp PUBLIC lsp)
  target_link_libraries(abnfLsp PUBLIC abnfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(abnf_lsp lsp.cpp)
    target_link_libraries(abnf_lsp PRIVATE abnfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET abnfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(abnf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(abnf)
