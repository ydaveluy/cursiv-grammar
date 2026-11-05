# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID pbm
    GRAMMAR    pbm/PbmGrammar.hpp
    NAMESPACE  pbm::Pbm
    EXTENSIONS .pbm
    OUT        pbm/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(pbmCore ${_core})
target_link_libraries(pbmCore PUBLIC pbmGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(pbm_cli cli.cpp)
  target_include_directories(pbm_cli PRIVATE cli)
  target_link_libraries(pbm_cli PRIVATE pbmCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(pbmCli ${_cli})
    target_include_directories(pbmCli PUBLIC cli)
    target_link_libraries(pbmCli PUBLIC pbmCore cursiv::cli)
    target_link_libraries(pbm_cli PRIVATE pbmCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(pbmLsp ${_lsp})
  target_include_directories(pbmLsp PUBLIC lsp)
  target_link_libraries(pbmLsp PUBLIC pbmCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(pbm_lsp lsp.cpp)
    target_link_libraries(pbm_lsp PRIVATE pbmLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET pbmLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(pbm SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(pbm)
