# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID vhdl
    GRAMMAR    vhdl/VhdlGrammar.hpp
    NAMESPACE  vhdl::Vhdl
    EXTENSIONS .vhd
    OUT        vhdl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(vhdlCore ${_core})
target_link_libraries(vhdlCore PUBLIC vhdlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(vhdl_cli cli.cpp)
  target_include_directories(vhdl_cli PRIVATE cli)
  target_link_libraries(vhdl_cli PRIVATE vhdlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(vhdlCli ${_cli})
    target_include_directories(vhdlCli PUBLIC cli)
    target_link_libraries(vhdlCli PUBLIC vhdlCore cursiv::cli)
    target_link_libraries(vhdl_cli PRIVATE vhdlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(vhdlLsp ${_lsp})
  target_include_directories(vhdlLsp PUBLIC lsp)
  target_link_libraries(vhdlLsp PUBLIC vhdlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(vhdl_lsp lsp.cpp)
    target_link_libraries(vhdl_lsp PRIVATE vhdlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET vhdlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(vhdl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(vhdl)
