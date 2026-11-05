# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID turtle
    GRAMMAR    turtle/TurtleGrammar.hpp
    NAMESPACE  turtle::Turtle
    EXTENSIONS .ttl
    OUT        turtle/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(turtleCore ${_core})
target_link_libraries(turtleCore PUBLIC turtleGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(turtle_cli cli.cpp)
  target_include_directories(turtle_cli PRIVATE cli)
  target_link_libraries(turtle_cli PRIVATE turtleCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(turtleCli ${_cli})
    target_include_directories(turtleCli PUBLIC cli)
    target_link_libraries(turtleCli PUBLIC turtleCore cursiv::cli)
    target_link_libraries(turtle_cli PRIVATE turtleCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(turtleLsp ${_lsp})
  target_include_directories(turtleLsp PUBLIC lsp)
  target_link_libraries(turtleLsp PUBLIC turtleCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(turtle_lsp lsp.cpp)
    target_link_libraries(turtle_lsp PRIVATE turtleLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET turtleLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(turtle SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(turtle)
