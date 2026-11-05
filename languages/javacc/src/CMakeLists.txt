# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID javacc
    GRAMMAR    javacc/JavaccGrammar.hpp
    NAMESPACE  javacc::Javacc
    EXTENSIONS .jj .jjt
    OUT        javacc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(javaccCore ${_core})
target_link_libraries(javaccCore PUBLIC javaccGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(javacc_cli cli.cpp)
  target_include_directories(javacc_cli PRIVATE cli)
  target_link_libraries(javacc_cli PRIVATE javaccCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(javaccCli ${_cli})
    target_include_directories(javaccCli PUBLIC cli)
    target_link_libraries(javaccCli PUBLIC javaccCore cursiv::cli)
    target_link_libraries(javacc_cli PRIVATE javaccCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(javaccLsp ${_lsp})
  target_include_directories(javaccLsp PUBLIC lsp)
  target_link_libraries(javaccLsp PUBLIC javaccCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(javacc_lsp lsp.cpp)
    target_link_libraries(javacc_lsp PRIVATE javaccLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET javaccLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(javacc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(javacc)
