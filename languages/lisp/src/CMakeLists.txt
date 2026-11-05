# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lisp
    GRAMMAR    lisp/LispGrammar.hpp
    NAMESPACE  lisp::Lisp
    EXTENSIONS .lisp
    OUT        lisp/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lispCore ${_core})
target_link_libraries(lispCore PUBLIC lispGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lisp_cli cli.cpp)
  target_include_directories(lisp_cli PRIVATE cli)
  target_link_libraries(lisp_cli PRIVATE lispCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lispCli ${_cli})
    target_include_directories(lispCli PUBLIC cli)
    target_link_libraries(lispCli PUBLIC lispCore cursiv::cli)
    target_link_libraries(lisp_cli PRIVATE lispCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lispLsp ${_lsp})
  target_include_directories(lispLsp PUBLIC lsp)
  target_link_libraries(lispLsp PUBLIC lispCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lisp_lsp lsp.cpp)
    target_link_libraries(lisp_lsp PRIVATE lispLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lispLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lisp SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lisp)
