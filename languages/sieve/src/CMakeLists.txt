# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sieveGenerated` (the compiled library) and `sieve_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sieve
    GRAMMAR    sieve/core/SieveGrammar.hpp
    NAMESPACE  sieve::Sieve
    EXTENSIONS .sieve
    OUT        sieve/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sieve/core/*.cpp)
add_library(sieveCore ${_core})
target_link_libraries(sieveCore PUBLIC sieveGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sieve/cli/*.cpp)
add_executable(sieve_cli ${_cli})
target_link_libraries(sieve_cli
                      PRIVATE sieveCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sieve/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sieve/lsp/main.cpp)
add_library(sieveLsp ${_lsp})
target_link_libraries(sieveLsp PUBLIC sieveCore cursiv::lsp)
add_executable(sieve_lsp sieve/lsp/main.cpp)
target_link_libraries(sieve_lsp PRIVATE sieveLsp)

cursiv_add_benchmark(sieve SOURCES sieve/bench/main.cpp)

cursiv_package_language(sieve)
