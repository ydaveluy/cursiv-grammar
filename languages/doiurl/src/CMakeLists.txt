# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID doiurl
    GRAMMAR    doiurl/DoiurlGrammar.hpp
    NAMESPACE  doiurl::Doiurl
    EXTENSIONS .doiurl
    OUT        doiurl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(doiurlCore ${_core})
target_link_libraries(doiurlCore PUBLIC doiurlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(doiurl_cli cli.cpp)
  target_include_directories(doiurl_cli PRIVATE cli)
  target_link_libraries(doiurl_cli PRIVATE doiurlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(doiurlCli ${_cli})
    target_include_directories(doiurlCli PUBLIC cli)
    target_link_libraries(doiurlCli PUBLIC doiurlCore cursiv::cli)
    target_link_libraries(doiurl_cli PRIVATE doiurlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(doiurlLsp ${_lsp})
  target_include_directories(doiurlLsp PUBLIC lsp)
  target_link_libraries(doiurlLsp PUBLIC doiurlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(doiurl_lsp lsp.cpp)
    target_link_libraries(doiurl_lsp PRIVATE doiurlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET doiurlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(doiurl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(doiurl)
