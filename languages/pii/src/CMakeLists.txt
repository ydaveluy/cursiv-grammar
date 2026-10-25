# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `piiGenerated` (the compiled library) and `pii_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pii
    GRAMMAR    pii/core/PiiGrammar.hpp
    NAMESPACE  pii::Pii
    EXTENSIONS .pii
    OUT        pii/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pii/core/*.cpp)
add_library(piiCore ${_core})
target_link_libraries(piiCore PUBLIC piiGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pii/cli/*.cpp)
add_executable(pii_cli ${_cli})
target_link_libraries(pii_cli
                      PRIVATE piiCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pii/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pii/lsp/main.cpp)
add_library(piiLsp ${_lsp})
target_link_libraries(piiLsp PUBLIC piiCore cursiv::lsp)
add_executable(pii_lsp pii/lsp/main.cpp)
target_link_libraries(pii_lsp PRIVATE piiLsp)

cursiv_add_benchmark(pii SOURCES pii/bench/main.cpp)

cursiv_package_language(pii)
