# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `arithmeticGenerated` (the compiled library) and `arithmetic_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID arithmetic
    GRAMMAR    arithmetic/core/ArithmeticGrammar.hpp
    NAMESPACE  arithmetic::Arithmetic
    EXTENSIONS .arithmetic
    OUT        arithmetic/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/core/*.cpp)
add_library(arithmeticCore ${_core})
target_link_libraries(arithmeticCore PUBLIC arithmeticGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/cli/*.cpp)
add_executable(arithmetic_cli ${_cli})
target_link_libraries(arithmetic_cli
                      PRIVATE arithmeticCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/lsp/main.cpp)
add_library(arithmeticLsp ${_lsp})
target_link_libraries(arithmeticLsp PUBLIC arithmeticCore cursiv::lsp)
add_executable(arithmetic_lsp arithmetic/lsp/main.cpp)
target_link_libraries(arithmetic_lsp PRIVATE arithmeticLsp)

cursiv_add_benchmark(arithmetic SOURCES arithmetic/bench/main.cpp)

cursiv_package_language(arithmetic)
