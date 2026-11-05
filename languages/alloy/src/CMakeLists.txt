# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID alloy
    GRAMMAR    alloy/AlloyGrammar.hpp
    NAMESPACE  alloy::Alloy
    EXTENSIONS .als
    OUT        alloy/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(alloyCore ${_core})
target_link_libraries(alloyCore PUBLIC alloyGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(alloy_cli cli.cpp)
  target_include_directories(alloy_cli PRIVATE cli)
  target_link_libraries(alloy_cli PRIVATE alloyCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(alloyCli ${_cli})
    target_include_directories(alloyCli PUBLIC cli)
    target_link_libraries(alloyCli PUBLIC alloyCore cursiv::cli)
    target_link_libraries(alloy_cli PRIVATE alloyCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(alloyLsp ${_lsp})
  target_include_directories(alloyLsp PUBLIC lsp)
  target_link_libraries(alloyLsp PUBLIC alloyCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(alloy_lsp lsp.cpp)
    target_link_libraries(alloy_lsp PRIVATE alloyLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET alloyLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(alloy SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(alloy)
