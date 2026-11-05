# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID peoplecode
    GRAMMAR    peoplecode/PeoplecodeGrammar.hpp
    NAMESPACE  peoplecode::Peoplecode
    EXTENSIONS .pc
    OUT        peoplecode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(peoplecodeCore ${_core})
target_link_libraries(peoplecodeCore PUBLIC peoplecodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(peoplecode_cli cli.cpp)
  target_include_directories(peoplecode_cli PRIVATE cli)
  target_link_libraries(peoplecode_cli PRIVATE peoplecodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(peoplecodeCli ${_cli})
    target_include_directories(peoplecodeCli PUBLIC cli)
    target_link_libraries(peoplecodeCli PUBLIC peoplecodeCore cursiv::cli)
    target_link_libraries(peoplecode_cli PRIVATE peoplecodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(peoplecodeLsp ${_lsp})
  target_include_directories(peoplecodeLsp PUBLIC lsp)
  target_link_libraries(peoplecodeLsp PUBLIC peoplecodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(peoplecode_lsp lsp.cpp)
    target_link_libraries(peoplecode_lsp PRIVATE peoplecodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET peoplecodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(peoplecode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(peoplecode)
