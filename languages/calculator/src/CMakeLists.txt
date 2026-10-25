# Generate the parser and its language module.
cursiv_generate_language(ID calculator
    GRAMMAR    calculator/core/CalculatorGrammar.hpp
    NAMESPACE  calculator::Calculator
    EXTENSIONS .calc
    OUT        calculator/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/calculator/core/*.cpp)
add_library(calculatorCore ${_core})
target_link_libraries(calculatorCore PUBLIC calculatorGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/calculator/cli/*.cpp)
if(_cli)
  add_executable(calculator_cli ${_cli})
  target_link_libraries(calculator_cli
                        PRIVATE calculatorCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/calculator/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/calculator/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/calculator/lsp/main.cpp)
  add_library(calculatorLsp ${_lsp})
  target_link_libraries(calculatorLsp PUBLIC calculatorCore cursiv::lsp)
  add_executable(calculator_lsp calculator/lsp/main.cpp)
  target_link_libraries(calculator_lsp PRIVATE calculatorLsp)
endif()

if(TARGET calculatorLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/calculator/bench/main.cpp")
  cursiv_add_benchmark(calculator
      SOURCES calculator/bench/main.cpp)
endif()

cursiv_package_language(calculator)
