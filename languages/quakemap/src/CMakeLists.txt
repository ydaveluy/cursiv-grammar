# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `quakemapGenerated` (the compiled library) and `quakemap_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID quakemap
    GRAMMAR    quakemap/core/QuakemapGrammar.hpp
    NAMESPACE  quakemap::Quakemap
    EXTENSIONS .map
    OUT        quakemap/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/core/*.cpp)
add_library(quakemapCore ${_core})
target_link_libraries(quakemapCore PUBLIC quakemapGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/cli/*.cpp)
add_executable(quakemap_cli ${_cli})
target_link_libraries(quakemap_cli
                      PRIVATE quakemapCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/lsp/main.cpp)
add_library(quakemapLsp ${_lsp})
target_link_libraries(quakemapLsp PUBLIC quakemapCore cursiv::lsp)
add_executable(quakemap_lsp quakemap/lsp/main.cpp)
target_link_libraries(quakemap_lsp PRIVATE quakemapLsp)

cursiv_add_benchmark(quakemap SOURCES quakemap/bench/main.cpp)

cursiv_package_language(quakemap)
