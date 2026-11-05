# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID scala
    GRAMMAR    scala/ScalaGrammar.hpp
    NAMESPACE  scala::Scala
    EXTENSIONS .scala
    OUT        scala/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(scalaCore ${_core})
target_link_libraries(scalaCore PUBLIC scalaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(scala_cli cli.cpp)
  target_include_directories(scala_cli PRIVATE cli)
  target_link_libraries(scala_cli PRIVATE scalaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(scalaCli ${_cli})
    target_include_directories(scalaCli PUBLIC cli)
    target_link_libraries(scalaCli PUBLIC scalaCore cursiv::cli)
    target_link_libraries(scala_cli PRIVATE scalaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(scalaLsp ${_lsp})
  target_include_directories(scalaLsp PUBLIC lsp)
  target_link_libraries(scalaLsp PUBLIC scalaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(scala_lsp lsp.cpp)
    target_link_libraries(scala_lsp PRIVATE scalaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET scalaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(scala SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(scala)
