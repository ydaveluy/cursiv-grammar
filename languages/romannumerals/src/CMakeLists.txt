# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `romannumeralsGenerated` (the compiled library) and `romannumerals_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID romannumerals
    GRAMMAR    romannumerals/core/RomannumeralsGrammar.hpp
    NAMESPACE  romannumerals::Romannumerals
    EXTENSIONS .romannumerals
    OUT        romannumerals/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/core/*.cpp)
add_library(romannumeralsCore ${_core})
target_link_libraries(romannumeralsCore PUBLIC romannumeralsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/cli/*.cpp)
add_executable(romannumerals_cli ${_cli})
target_link_libraries(romannumerals_cli
                      PRIVATE romannumeralsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/lsp/main.cpp)
add_library(romannumeralsLsp ${_lsp})
target_link_libraries(romannumeralsLsp PUBLIC romannumeralsCore cursiv::lsp)
add_executable(romannumerals_lsp romannumerals/lsp/main.cpp)
target_link_libraries(romannumerals_lsp PRIVATE romannumeralsLsp)

cursiv_add_benchmark(romannumerals SOURCES romannumerals/bench/main.cpp)

cursiv_package_language(romannumerals)
