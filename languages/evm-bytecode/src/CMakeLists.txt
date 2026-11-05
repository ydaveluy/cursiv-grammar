# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID evmbytecode
    GRAMMAR    evmbytecode/EvmBytecodeGrammar.hpp
    NAMESPACE  evmbytecode::EvmBytecode
    EXTENSIONS .evm
    OUT        evmbytecode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(evmbytecodeCore ${_core})
target_link_libraries(evmbytecodeCore PUBLIC evmbytecodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(evmbytecode_cli cli.cpp)
  target_include_directories(evmbytecode_cli PRIVATE cli)
  target_link_libraries(evmbytecode_cli PRIVATE evmbytecodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(evmbytecodeCli ${_cli})
    target_include_directories(evmbytecodeCli PUBLIC cli)
    target_link_libraries(evmbytecodeCli PUBLIC evmbytecodeCore cursiv::cli)
    target_link_libraries(evmbytecode_cli PRIVATE evmbytecodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(evmbytecodeLsp ${_lsp})
  target_include_directories(evmbytecodeLsp PUBLIC lsp)
  target_link_libraries(evmbytecodeLsp PUBLIC evmbytecodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(evmbytecode_lsp lsp.cpp)
    target_link_libraries(evmbytecode_lsp PRIVATE evmbytecodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET evmbytecodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(evmbytecode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(evmbytecode)
