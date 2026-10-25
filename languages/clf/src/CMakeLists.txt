# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `clfGenerated` (the compiled library) and `clf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID clf
    GRAMMAR    clf/core/ClfGrammar.hpp
    NAMESPACE  clf::Clf
    EXTENSIONS .clf
    OUT        clf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clf/core/*.cpp)
add_library(clfCore ${_core})
target_link_libraries(clfCore PUBLIC clfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clf/cli/*.cpp)
add_executable(clf_cli ${_cli})
target_link_libraries(clf_cli
                      PRIVATE clfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clf/lsp/main.cpp)
add_library(clfLsp ${_lsp})
target_link_libraries(clfLsp PUBLIC clfCore cursiv::lsp)
add_executable(clf_lsp clf/lsp/main.cpp)
target_link_libraries(clf_lsp PRIVATE clfLsp)

cursiv_add_benchmark(clf SOURCES clf/bench/main.cpp)

cursiv_package_language(clf)
