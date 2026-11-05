# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lcc
    GRAMMAR    lcc/LccGrammar.hpp
    NAMESPACE  lcc::Lcc
    EXTENSIONS .lcc
    OUT        lcc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lccCore ${_core})
target_link_libraries(lccCore PUBLIC lccGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lcc_cli cli.cpp)
  target_include_directories(lcc_cli PRIVATE cli)
  target_link_libraries(lcc_cli PRIVATE lccCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lccCli ${_cli})
    target_include_directories(lccCli PUBLIC cli)
    target_link_libraries(lccCli PUBLIC lccCore cursiv::cli)
    target_link_libraries(lcc_cli PRIVATE lccCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lccLsp ${_lsp})
  target_include_directories(lccLsp PUBLIC lsp)
  target_link_libraries(lccLsp PUBLIC lccCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lcc_lsp lsp.cpp)
    target_link_libraries(lcc_lsp PRIVATE lccLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lccLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lcc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lcc)
