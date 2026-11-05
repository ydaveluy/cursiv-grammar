# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID robotwars
    GRAMMAR    robotwars/RobotwarsGrammar.hpp
    NAMESPACE  robotwars::Robotwars
    EXTENSIONS .robotwars
    OUT        robotwars/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(robotwarsCore ${_core})
target_link_libraries(robotwarsCore PUBLIC robotwarsGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(robotwars_cli cli.cpp)
  target_include_directories(robotwars_cli PRIVATE cli)
  target_link_libraries(robotwars_cli PRIVATE robotwarsCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(robotwarsCli ${_cli})
    target_include_directories(robotwarsCli PUBLIC cli)
    target_link_libraries(robotwarsCli PUBLIC robotwarsCore cursiv::cli)
    target_link_libraries(robotwars_cli PRIVATE robotwarsCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(robotwarsLsp ${_lsp})
  target_include_directories(robotwarsLsp PUBLIC lsp)
  target_link_libraries(robotwarsLsp PUBLIC robotwarsCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(robotwars_lsp lsp.cpp)
    target_link_libraries(robotwars_lsp PRIVATE robotwarsLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET robotwarsLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(robotwars SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(robotwars)
