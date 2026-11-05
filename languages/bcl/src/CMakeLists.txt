# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID bcl
    GRAMMAR    bcl/BclGrammar.hpp
    NAMESPACE  bcl::Bcl
    EXTENSIONS .bcl
    OUT        bcl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(bclCore ${_core})
target_link_libraries(bclCore PUBLIC bclGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(bcl_cli cli.cpp)
  target_include_directories(bcl_cli PRIVATE cli)
  target_link_libraries(bcl_cli PRIVATE bclCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(bclCli ${_cli})
    target_include_directories(bclCli PUBLIC cli)
    target_link_libraries(bclCli PUBLIC bclCore cursiv::cli)
    target_link_libraries(bcl_cli PRIVATE bclCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(bclLsp ${_lsp})
  target_include_directories(bclLsp PUBLIC lsp)
  target_link_libraries(bclLsp PUBLIC bclCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(bcl_lsp lsp.cpp)
    target_link_libraries(bcl_lsp PRIVATE bclLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET bclLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(bcl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(bcl)
