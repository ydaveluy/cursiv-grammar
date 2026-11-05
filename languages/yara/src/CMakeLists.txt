# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID yara
    GRAMMAR    yara/YaraGrammar.hpp
    NAMESPACE  yara::Yara
    EXTENSIONS .yar
    OUT        yara/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(yaraCore ${_core})
target_link_libraries(yaraCore PUBLIC yaraGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(yara_cli cli.cpp)
  target_include_directories(yara_cli PRIVATE cli)
  target_link_libraries(yara_cli PRIVATE yaraCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(yaraCli ${_cli})
    target_include_directories(yaraCli PUBLIC cli)
    target_link_libraries(yaraCli PUBLIC yaraCore cursiv::cli)
    target_link_libraries(yara_cli PRIVATE yaraCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(yaraLsp ${_lsp})
  target_include_directories(yaraLsp PUBLIC lsp)
  target_link_libraries(yaraLsp PUBLIC yaraCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(yara_lsp lsp.cpp)
    target_link_libraries(yara_lsp PRIVATE yaraLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET yaraLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(yara SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(yara)
