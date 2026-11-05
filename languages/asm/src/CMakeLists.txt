# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID nasm
    GRAMMAR    nasm/NasmGrammar.hpp
    NAMESPACE  nasm::Nasm
    EXTENSIONS .asm
    OUT        nasm/generated)

# The mnemonic and register vocabularies alone are ~2450 terminals, past the
# default 8-word (512-bit) ceiling of the generator's per-state terminal
# sets; cursivLrGen (linked into nasm_gen) builds the tables, so it needs
# the same raised ceiling.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_definitions(nasm_gen PRIVATE CURSIV_SET_WORDS=64u)")
  cursiv_host_manifest("target_compile_definitions(cursivLrGen PRIVATE CURSIV_SET_WORDS=64u)")
else()
  target_compile_definitions(nasm_gen PRIVATE CURSIV_SET_WORDS=64u)
  if(TARGET cursivLrGen)
    target_compile_definitions(cursivLrGen PRIVATE CURSIV_SET_WORDS=64u)
  endif()
endif()

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(nasmCore ${_core})
target_link_libraries(nasmCore PUBLIC nasmGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(nasm_cli cli.cpp)
  target_include_directories(nasm_cli PRIVATE cli)
  target_link_libraries(nasm_cli PRIVATE nasmCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(nasmCli ${_cli})
    target_include_directories(nasmCli PUBLIC cli)
    target_link_libraries(nasmCli PUBLIC nasmCore cursiv::cli)
    target_link_libraries(nasm_cli PRIVATE nasmCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(nasmLsp ${_lsp})
  target_include_directories(nasmLsp PUBLIC lsp)
  target_link_libraries(nasmLsp PUBLIC nasmCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(nasm_lsp lsp.cpp)
    target_link_libraries(nasm_lsp PRIVATE nasmLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET nasmLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(nasm SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(nasm)
