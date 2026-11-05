# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID kotlin
    GRAMMAR    kotlin/KotlinGrammar.hpp
    NAMESPACE  kotlin::Kotlin
    EXTENSIONS .kt
    OUT        kotlin/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(kotlinCore ${_core})
target_link_libraries(kotlinCore PUBLIC kotlinGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(kotlin_cli cli.cpp)
  target_include_directories(kotlin_cli PRIVATE cli)
  target_link_libraries(kotlin_cli PRIVATE kotlinCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(kotlinCli ${_cli})
    target_include_directories(kotlinCli PUBLIC cli)
    target_link_libraries(kotlinCli PUBLIC kotlinCore cursiv::cli)
    target_link_libraries(kotlin_cli PRIVATE kotlinCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(kotlinLsp ${_lsp})
  target_include_directories(kotlinLsp PUBLIC lsp)
  target_link_libraries(kotlinLsp PUBLIC kotlinCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(kotlin_lsp lsp.cpp)
    target_link_libraries(kotlin_lsp PRIVATE kotlinLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET kotlinLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(kotlin SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(kotlin)
