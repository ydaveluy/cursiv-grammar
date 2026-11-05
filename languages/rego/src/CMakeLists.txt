# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rego
    GRAMMAR    rego/RegoGrammar.hpp
    NAMESPACE  rego::Rego
    EXTENSIONS .rego
    OUT        rego/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(regoCore ${_core})
target_link_libraries(regoCore PUBLIC regoGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rego_cli cli.cpp)
  target_include_directories(rego_cli PRIVATE cli)
  target_link_libraries(rego_cli PRIVATE regoCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(regoCli ${_cli})
    target_include_directories(regoCli PUBLIC cli)
    target_link_libraries(regoCli PUBLIC regoCore cursiv::cli)
    target_link_libraries(rego_cli PRIVATE regoCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(regoLsp ${_lsp})
  target_include_directories(regoLsp PUBLIC lsp)
  target_link_libraries(regoLsp PUBLIC regoCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rego_lsp lsp.cpp)
    target_link_libraries(rego_lsp PRIVATE regoLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET regoLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rego SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rego)
