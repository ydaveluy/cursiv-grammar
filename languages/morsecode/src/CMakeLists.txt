# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID morsecode
    GRAMMAR    morsecode/MorsecodeGrammar.hpp
    NAMESPACE  morsecode::Morsecode
    EXTENSIONS .morsecode
    OUT        morsecode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(morsecodeCore ${_core})
target_link_libraries(morsecodeCore PUBLIC morsecodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(morsecode_cli cli.cpp)
  target_include_directories(morsecode_cli PRIVATE cli)
  target_link_libraries(morsecode_cli PRIVATE morsecodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(morsecodeCli ${_cli})
    target_include_directories(morsecodeCli PUBLIC cli)
    target_link_libraries(morsecodeCli PUBLIC morsecodeCore cursiv::cli)
    target_link_libraries(morsecode_cli PRIVATE morsecodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(morsecodeLsp ${_lsp})
  target_include_directories(morsecodeLsp PUBLIC lsp)
  target_link_libraries(morsecodeLsp PUBLIC morsecodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(morsecode_lsp lsp.cpp)
    target_link_libraries(morsecode_lsp PRIVATE morsecodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET morsecodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(morsecode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(morsecode)
