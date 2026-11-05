# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID clojure
    GRAMMAR    clojure/ClojureGrammar.hpp
    NAMESPACE  clojure::Clojure
    EXTENSIONS .clj
    OUT        clojure/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(clojureCore ${_core})
target_link_libraries(clojureCore PUBLIC clojureGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(clojure_cli cli.cpp)
  target_include_directories(clojure_cli PRIVATE cli)
  target_link_libraries(clojure_cli PRIVATE clojureCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(clojureCli ${_cli})
    target_include_directories(clojureCli PUBLIC cli)
    target_link_libraries(clojureCli PUBLIC clojureCore cursiv::cli)
    target_link_libraries(clojure_cli PRIVATE clojureCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(clojureLsp ${_lsp})
  target_include_directories(clojureLsp PUBLIC lsp)
  target_link_libraries(clojureLsp PUBLIC clojureCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(clojure_lsp lsp.cpp)
    target_link_libraries(clojure_lsp PRIVATE clojureLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET clojureLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(clojure SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(clojure)
