# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID bencoding
    GRAMMAR    bencoding/BencodingGrammar.hpp
    NAMESPACE  bencoding::Bencoding
    EXTENSIONS .torrent
    OUT        bencoding/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(bencodingCore ${_core})
target_link_libraries(bencodingCore PUBLIC bencodingGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(bencoding_cli cli.cpp)
  target_include_directories(bencoding_cli PRIVATE cli)
  target_link_libraries(bencoding_cli PRIVATE bencodingCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(bencodingCli ${_cli})
    target_include_directories(bencodingCli PUBLIC cli)
    target_link_libraries(bencodingCli PUBLIC bencodingCore cursiv::cli)
    target_link_libraries(bencoding_cli PRIVATE bencodingCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(bencodingLsp ${_lsp})
  target_include_directories(bencodingLsp PUBLIC lsp)
  target_link_libraries(bencodingLsp PUBLIC bencodingCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(bencoding_lsp lsp.cpp)
    target_link_libraries(bencoding_lsp PRIVATE bencodingLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET bencodingLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(bencoding SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(bencoding)
