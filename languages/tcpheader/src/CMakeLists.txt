# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tcpheader
    GRAMMAR    tcpheader/TcpheaderGrammar.hpp
    NAMESPACE  tcpheader::Tcpheader
    EXTENSIONS .tcpheader
    OUT        tcpheader/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tcpheaderCore ${_core})
target_link_libraries(tcpheaderCore PUBLIC tcpheaderGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tcpheader_cli cli.cpp)
  target_include_directories(tcpheader_cli PRIVATE cli)
  target_link_libraries(tcpheader_cli PRIVATE tcpheaderCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tcpheaderCli ${_cli})
    target_include_directories(tcpheaderCli PUBLIC cli)
    target_link_libraries(tcpheaderCli PUBLIC tcpheaderCore cursiv::cli)
    target_link_libraries(tcpheader_cli PRIVATE tcpheaderCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tcpheaderLsp ${_lsp})
  target_include_directories(tcpheaderLsp PUBLIC lsp)
  target_link_libraries(tcpheaderLsp PUBLIC tcpheaderCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tcpheader_lsp lsp.cpp)
    target_link_libraries(tcpheader_lsp PRIVATE tcpheaderLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tcpheaderLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tcpheader SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tcpheader)
