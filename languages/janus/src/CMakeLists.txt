# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID janus
    GRAMMAR    janus/JanusGrammar.hpp
    NAMESPACE  janus::Janus
    EXTENSIONS .janus
    OUT        janus/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(janusCore ${_core})
target_link_libraries(janusCore PUBLIC janusGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(janus_cli cli.cpp)
  target_include_directories(janus_cli PRIVATE cli)
  target_link_libraries(janus_cli PRIVATE janusCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(janusCli ${_cli})
    target_include_directories(janusCli PUBLIC cli)
    target_link_libraries(janusCli PUBLIC janusCore cursiv::cli)
    target_link_libraries(janus_cli PRIVATE janusCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(janusLsp ${_lsp})
  target_include_directories(janusLsp PUBLIC lsp)
  target_link_libraries(janusLsp PUBLIC janusCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(janus_lsp lsp.cpp)
    target_link_libraries(janus_lsp PRIVATE janusLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET janusLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(janus SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(janus)
