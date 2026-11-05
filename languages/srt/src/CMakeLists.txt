# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID srt
    GRAMMAR    srt/SrtGrammar.hpp
    NAMESPACE  srt::Srt
    EXTENSIONS .srt
    OUT        srt/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(srtCore ${_core})
target_link_libraries(srtCore PUBLIC srtGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(srt_cli cli.cpp)
  target_include_directories(srt_cli PRIVATE cli)
  target_link_libraries(srt_cli PRIVATE srtCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(srtCli ${_cli})
    target_include_directories(srtCli PUBLIC cli)
    target_link_libraries(srtCli PUBLIC srtCore cursiv::cli)
    target_link_libraries(srt_cli PRIVATE srtCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(srtLsp ${_lsp})
  target_include_directories(srtLsp PUBLIC lsp)
  target_link_libraries(srtLsp PUBLIC srtCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(srt_lsp lsp.cpp)
    target_link_libraries(srt_lsp PRIVATE srtLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET srtLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(srt SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(srt)
