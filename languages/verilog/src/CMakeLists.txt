# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID verilog
    GRAMMAR    verilog/VerilogGrammar.hpp
    NAMESPACE  verilog::Verilog
    EXTENSIONS .v
    OUT        verilog/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(verilogCore ${_core})
target_link_libraries(verilogCore PUBLIC verilogGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(verilog_cli cli.cpp)
  target_include_directories(verilog_cli PRIVATE cli)
  target_link_libraries(verilog_cli PRIVATE verilogCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(verilogCli ${_cli})
    target_include_directories(verilogCli PUBLIC cli)
    target_link_libraries(verilogCli PUBLIC verilogCore cursiv::cli)
    target_link_libraries(verilog_cli PRIVATE verilogCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(verilogLsp ${_lsp})
  target_include_directories(verilogLsp PUBLIC lsp)
  target_link_libraries(verilogLsp PUBLIC verilogCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(verilog_lsp lsp.cpp)
    target_link_libraries(verilog_lsp PRIVATE verilogLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET verilogLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(verilog SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(verilog)
