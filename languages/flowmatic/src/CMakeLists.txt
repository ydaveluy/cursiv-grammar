# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID flowmatic
    GRAMMAR    flowmatic/FlowmaticGrammar.hpp
    NAMESPACE  flowmatic::Flowmatic
    EXTENSIONS .flowmatic
    OUT        flowmatic/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(flowmaticCore ${_core})
target_link_libraries(flowmaticCore PUBLIC flowmaticGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(flowmatic_cli cli.cpp)
  target_include_directories(flowmatic_cli PRIVATE cli)
  target_link_libraries(flowmatic_cli PRIVATE flowmaticCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(flowmaticCli ${_cli})
    target_include_directories(flowmaticCli PUBLIC cli)
    target_link_libraries(flowmaticCli PUBLIC flowmaticCore cursiv::cli)
    target_link_libraries(flowmatic_cli PRIVATE flowmaticCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(flowmaticLsp ${_lsp})
  target_include_directories(flowmaticLsp PUBLIC lsp)
  target_link_libraries(flowmaticLsp PUBLIC flowmaticCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(flowmatic_lsp lsp.cpp)
    target_link_libraries(flowmatic_lsp PRIVATE flowmaticLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET flowmaticLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(flowmatic SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(flowmatic)
