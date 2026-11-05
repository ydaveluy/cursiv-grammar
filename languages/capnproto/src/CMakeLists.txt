# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID capnproto
    GRAMMAR    capnproto/CapnprotoGrammar.hpp
    NAMESPACE  capnproto::Capnproto
    EXTENSIONS .capnp
    OUT        capnproto/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(capnprotoCore ${_core})
target_link_libraries(capnprotoCore PUBLIC capnprotoGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(capnproto_cli cli.cpp)
  target_include_directories(capnproto_cli PRIVATE cli)
  target_link_libraries(capnproto_cli PRIVATE capnprotoCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(capnprotoCli ${_cli})
    target_include_directories(capnprotoCli PUBLIC cli)
    target_link_libraries(capnprotoCli PUBLIC capnprotoCore cursiv::cli)
    target_link_libraries(capnproto_cli PRIVATE capnprotoCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(capnprotoLsp ${_lsp})
  target_include_directories(capnprotoLsp PUBLIC lsp)
  target_link_libraries(capnprotoLsp PUBLIC capnprotoCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(capnproto_lsp lsp.cpp)
    target_link_libraries(capnproto_lsp PRIVATE capnprotoLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET capnprotoLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(capnproto SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(capnproto)
