# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lambda
    GRAMMAR    lambda/LambdaGrammar.hpp
    NAMESPACE  lambda::Lambda
    EXTENSIONS .lambda
    OUT        lambda/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lambdaCore ${_core})
target_link_libraries(lambdaCore PUBLIC lambdaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lambda_cli cli.cpp)
  target_include_directories(lambda_cli PRIVATE cli)
  target_link_libraries(lambda_cli PRIVATE lambdaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lambdaCli ${_cli})
    target_include_directories(lambdaCli PUBLIC cli)
    target_link_libraries(lambdaCli PUBLIC lambdaCore cursiv::cli)
    target_link_libraries(lambda_cli PRIVATE lambdaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lambdaLsp ${_lsp})
  target_include_directories(lambdaLsp PUBLIC lsp)
  target_link_libraries(lambdaLsp PUBLIC lambdaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lambda_lsp lsp.cpp)
    target_link_libraries(lambda_lsp PRIVATE lambdaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lambdaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lambda SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lambda)
