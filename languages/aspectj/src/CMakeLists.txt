# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID aspectj
    GRAMMAR    aspectj/AspectjGrammar.hpp
    NAMESPACE  aspectj::Aspectj
    EXTENSIONS .aj
    OUT        aspectj/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(aspectjCore ${_core})
target_link_libraries(aspectjCore PUBLIC aspectjGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(aspectj_cli cli.cpp)
  target_include_directories(aspectj_cli PRIVATE cli)
  target_link_libraries(aspectj_cli PRIVATE aspectjCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(aspectjCli ${_cli})
    target_include_directories(aspectjCli PUBLIC cli)
    target_link_libraries(aspectjCli PUBLIC aspectjCore cursiv::cli)
    target_link_libraries(aspectj_cli PRIVATE aspectjCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(aspectjLsp ${_lsp})
  target_include_directories(aspectjLsp PUBLIC lsp)
  target_link_libraries(aspectjLsp PUBLIC aspectjCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(aspectj_lsp lsp.cpp)
    target_link_libraries(aspectj_lsp PRIVATE aspectjLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET aspectjLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(aspectj SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(aspectj)
