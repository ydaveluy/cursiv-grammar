# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `iriGenerated` (the compiled library) and `iri_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID iri
    GRAMMAR    iri/core/IriGrammar.hpp
    NAMESPACE  iri::Iri
    EXTENSIONS .iri
    OUT        iri/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iri/core/*.cpp)
add_library(iriCore ${_core})
target_link_libraries(iriCore PUBLIC iriGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iri/cli/*.cpp)
add_executable(iri_cli ${_cli})
target_link_libraries(iri_cli
                      PRIVATE iriCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iri/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/iri/lsp/main.cpp)
add_library(iriLsp ${_lsp})
target_link_libraries(iriLsp PUBLIC iriCore cursiv::lsp)
add_executable(iri_lsp iri/lsp/main.cpp)
target_link_libraries(iri_lsp PRIVATE iriLsp)

cursiv_add_benchmark(iri SOURCES iri/bench/main.cpp)

cursiv_package_language(iri)
