# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lolcode
    GRAMMAR    lolcode/LolcodeGrammar.hpp
    NAMESPACE  lolcode::Lolcode
    EXTENSIONS .lol
    OUT        lolcode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lolcodeCore ${_core})
target_link_libraries(lolcodeCore PUBLIC lolcodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lolcode_cli cli.cpp)
  target_include_directories(lolcode_cli PRIVATE cli)
  target_link_libraries(lolcode_cli PRIVATE lolcodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lolcodeCli ${_cli})
    target_include_directories(lolcodeCli PUBLIC cli)
    target_link_libraries(lolcodeCli PUBLIC lolcodeCore cursiv::cli)
    target_link_libraries(lolcode_cli PRIVATE lolcodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lolcodeLsp ${_lsp})
  target_include_directories(lolcodeLsp PUBLIC lsp)
  target_link_libraries(lolcodeLsp PUBLIC lolcodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lolcode_lsp lsp.cpp)
    target_link_libraries(lolcode_lsp PRIVATE lolcodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lolcodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lolcode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lolcode)
