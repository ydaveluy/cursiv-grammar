# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `calculatorGenerated` (the compiled library) and `calculator_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID calculator
    GRAMMAR    calculator/core/CalculatorGrammar.hpp
    NAMESPACE  calculator::Calculator
    EXTENSIONS .calc
    OUT        calculator/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/calculator/core/*.cpp)
add_library(calculatorCore ${_core})
target_link_libraries(calculatorCore PUBLIC calculatorGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/calculator/cli/*.cpp)
add_executable(calculator_cli ${_cli})
target_link_libraries(calculator_cli
                      PRIVATE calculatorCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/calculator/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/calculator/lsp/main.cpp)
add_library(calculatorLsp ${_lsp})
target_link_libraries(calculatorLsp PUBLIC calculatorCore cursiv::lsp)
add_executable(calculator_lsp calculator/lsp/main.cpp)
target_link_libraries(calculator_lsp PRIVATE calculatorLsp)

cursiv_add_benchmark(calculator SOURCES calculator/bench/main.cpp)

cursiv_package_language(calculator)
