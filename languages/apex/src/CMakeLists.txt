# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID apex
    GRAMMAR    apex/ApexGrammar.hpp
    NAMESPACE  apex::Apex
    EXTENSIONS .cls
    OUT        apex/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(apexCore ${_core})
target_link_libraries(apexCore PUBLIC apexGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(apex_cli cli.cpp)
  target_include_directories(apex_cli PRIVATE cli)
  target_link_libraries(apex_cli PRIVATE apexCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(apexCli ${_cli})
    target_include_directories(apexCli PUBLIC cli)
    target_link_libraries(apexCli PUBLIC apexCore cursiv::cli)
    target_link_libraries(apex_cli PRIVATE apexCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(apexLsp ${_lsp})
  target_include_directories(apexLsp PUBLIC lsp)
  target_link_libraries(apexLsp PUBLIC apexCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(apex_lsp lsp.cpp)
    target_link_libraries(apex_lsp PRIVATE apexLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET apexLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(apex SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(apex)
