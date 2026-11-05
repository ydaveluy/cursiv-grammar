# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID thrift
    GRAMMAR    thrift/ThriftGrammar.hpp
    NAMESPACE  thrift::Thrift
    EXTENSIONS .thrift
    OUT        thrift/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(thriftCore ${_core})
target_link_libraries(thriftCore PUBLIC thriftGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(thrift_cli cli.cpp)
  target_include_directories(thrift_cli PRIVATE cli)
  target_link_libraries(thrift_cli PRIVATE thriftCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(thriftCli ${_cli})
    target_include_directories(thriftCli PUBLIC cli)
    target_link_libraries(thriftCli PUBLIC thriftCore cursiv::cli)
    target_link_libraries(thrift_cli PRIVATE thriftCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(thriftLsp ${_lsp})
  target_include_directories(thriftLsp PUBLIC lsp)
  target_link_libraries(thriftLsp PUBLIC thriftCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(thrift_lsp lsp.cpp)
    target_link_libraries(thrift_lsp PRIVATE thriftLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET thriftLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(thrift SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(thrift)
