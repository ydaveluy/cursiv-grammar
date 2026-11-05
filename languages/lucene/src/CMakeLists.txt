# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lucene
    GRAMMAR    lucene/LuceneGrammar.hpp
    NAMESPACE  lucene::Lucene
    EXTENSIONS .lucene
    OUT        lucene/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(luceneCore ${_core})
target_link_libraries(luceneCore PUBLIC luceneGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lucene_cli cli.cpp)
  target_include_directories(lucene_cli PRIVATE cli)
  target_link_libraries(lucene_cli PRIVATE luceneCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(luceneCli ${_cli})
    target_include_directories(luceneCli PUBLIC cli)
    target_link_libraries(luceneCli PUBLIC luceneCore cursiv::cli)
    target_link_libraries(lucene_cli PRIVATE luceneCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(luceneLsp ${_lsp})
  target_include_directories(luceneLsp PUBLIC lsp)
  target_link_libraries(luceneLsp PUBLIC luceneCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lucene_lsp lsp.cpp)
    target_link_libraries(lucene_lsp PRIVATE luceneLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET luceneLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lucene SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lucene)
