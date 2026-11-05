# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID upnp
    GRAMMAR    upnp/UpnpGrammar.hpp
    NAMESPACE  upnp::Upnp
    EXTENSIONS .upnp
    OUT        upnp/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(upnpCore ${_core})
target_link_libraries(upnpCore PUBLIC upnpGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(upnp_cli cli.cpp)
  target_include_directories(upnp_cli PRIVATE cli)
  target_link_libraries(upnp_cli PRIVATE upnpCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(upnpCli ${_cli})
    target_include_directories(upnpCli PUBLIC cli)
    target_link_libraries(upnpCli PUBLIC upnpCore cursiv::cli)
    target_link_libraries(upnp_cli PRIVATE upnpCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(upnpLsp ${_lsp})
  target_include_directories(upnpLsp PUBLIC lsp)
  target_link_libraries(upnpLsp PUBLIC upnpCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(upnp_lsp lsp.cpp)
    target_link_libraries(upnp_lsp PRIVATE upnpLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET upnpLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(upnp SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(upnp)
