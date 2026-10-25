# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `asnGenerated` (the compiled library) and `asn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID asn
    GRAMMAR    asn/core/AsnGrammar.hpp
    NAMESPACE  asn::Asn
    EXTENSIONS .asn
    OUT        asn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asn/core/*.cpp)
add_library(asnCore ${_core})
target_link_libraries(asnCore PUBLIC asnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asn/cli/*.cpp)
add_executable(asn_cli ${_cli})
target_link_libraries(asn_cli
                      PRIVATE asnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/asn/lsp/main.cpp)
add_library(asnLsp ${_lsp})
target_link_libraries(asnLsp PUBLIC asnCore cursiv::lsp)
add_executable(asn_lsp asn/lsp/main.cpp)
target_link_libraries(asn_lsp PRIVATE asnLsp)

cursiv_add_benchmark(asn SOURCES asn/bench/main.cpp)

cursiv_package_language(asn)
