# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID url
    GRAMMAR    url/UrlGrammar.hpp
    NAMESPACE  url::Url
    EXTENSIONS .url
    OUT        url/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(urlCore ${_core})
target_link_libraries(urlCore PUBLIC urlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(url_cli cli.cpp)
  target_include_directories(url_cli PRIVATE cli)
  target_link_libraries(url_cli PRIVATE urlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(urlCli ${_cli})
    target_include_directories(urlCli PUBLIC cli)
    target_link_libraries(urlCli PUBLIC urlCore cursiv::cli)
    target_link_libraries(url_cli PRIVATE urlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(urlLsp ${_lsp})
  target_include_directories(urlLsp PUBLIC lsp)
  target_link_libraries(urlLsp PUBLIC urlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(url_lsp lsp.cpp)
    target_link_libraries(url_lsp PRIVATE urlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET urlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(url SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(url)
