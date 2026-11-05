# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID calculator
    GRAMMAR    calculator/CalculatorGrammar.hpp
    NAMESPACE  calculator::Calculator
    EXTENSIONS .calc
    OUT        calculator/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(calculatorCore ${_core})
target_link_libraries(calculatorCore PUBLIC calculatorGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(calculator_cli cli.cpp)
  target_include_directories(calculator_cli PRIVATE cli)
  target_link_libraries(calculator_cli PRIVATE calculatorCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(calculatorCli ${_cli})
    target_include_directories(calculatorCli PUBLIC cli)
    target_link_libraries(calculatorCli PUBLIC calculatorCore cursiv::cli)
    target_link_libraries(calculator_cli PRIVATE calculatorCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(calculatorLsp ${_lsp})
  target_include_directories(calculatorLsp PUBLIC lsp)
  target_link_libraries(calculatorLsp PUBLIC calculatorCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(calculator_lsp lsp.cpp)
    target_link_libraries(calculator_lsp PRIVATE calculatorLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET calculatorLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(calculator SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(calculator)
