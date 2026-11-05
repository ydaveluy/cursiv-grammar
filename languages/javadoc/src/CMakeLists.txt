# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID javadoc
    GRAMMAR    javadoc/JavadocGrammar.hpp
    NAMESPACE  javadoc::Javadoc
    EXTENSIONS .javadoc
    OUT        javadoc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(javadocCore ${_core})
target_link_libraries(javadocCore PUBLIC javadocGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(javadoc_cli cli.cpp)
  target_include_directories(javadoc_cli PRIVATE cli)
  target_link_libraries(javadoc_cli PRIVATE javadocCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(javadocCli ${_cli})
    target_include_directories(javadocCli PUBLIC cli)
    target_link_libraries(javadocCli PUBLIC javadocCore cursiv::cli)
    target_link_libraries(javadoc_cli PRIVATE javadocCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(javadocLsp ${_lsp})
  target_include_directories(javadocLsp PUBLIC lsp)
  target_link_libraries(javadocLsp PUBLIC javadocCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(javadoc_lsp lsp.cpp)
    target_link_libraries(javadoc_lsp PRIVATE javadocLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET javadocLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(javadoc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(javadoc)
