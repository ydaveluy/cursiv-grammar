# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `istcGenerated` (the compiled library) and `istc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID istc
    GRAMMAR    istc/core/IstcGrammar.hpp
    NAMESPACE  istc::Istc
    EXTENSIONS .istc
    OUT        istc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/istc/core/*.cpp)
add_library(istcCore ${_core})
target_link_libraries(istcCore PUBLIC istcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/istc/cli/*.cpp)
add_executable(istc_cli ${_cli})
target_link_libraries(istc_cli
                      PRIVATE istcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/istc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/istc/lsp/main.cpp)
add_library(istcLsp ${_lsp})
target_link_libraries(istcLsp PUBLIC istcCore cursiv::lsp)
add_executable(istc_lsp istc/lsp/main.cpp)
target_link_libraries(istc_lsp PRIVATE istcLsp)

cursiv_add_benchmark(istc SOURCES istc/bench/main.cpp)

cursiv_package_language(istc)
