# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID asn
    GRAMMAR    asn/AsnGrammar.hpp
    NAMESPACE  asn::Asn
    EXTENSIONS .asn
    OUT        asn/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(asnCore ${_core})
target_link_libraries(asnCore PUBLIC asnGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(asn_cli cli.cpp)
  target_include_directories(asn_cli PRIVATE cli)
  target_link_libraries(asn_cli PRIVATE asnCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(asnCli ${_cli})
    target_include_directories(asnCli PUBLIC cli)
    target_link_libraries(asnCli PUBLIC asnCore cursiv::cli)
    target_link_libraries(asn_cli PRIVATE asnCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(asnLsp ${_lsp})
  target_include_directories(asnLsp PUBLIC lsp)
  target_link_libraries(asnLsp PUBLIC asnCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(asn_lsp lsp.cpp)
    target_link_libraries(asn_lsp PRIVATE asnLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET asnLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(asn SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(asn)
