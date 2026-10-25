# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ednGenerated` (the compiled library) and `edn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID edn
    GRAMMAR    edn/core/EdnGrammar.hpp
    NAMESPACE  edn::Edn
    EXTENSIONS .edn
    OUT        edn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edn/core/*.cpp)
add_library(ednCore ${_core})
target_link_libraries(ednCore PUBLIC ednGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edn/cli/*.cpp)
add_executable(edn_cli ${_cli})
target_link_libraries(edn_cli
                      PRIVATE ednCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/edn/lsp/main.cpp)
add_library(ednLsp ${_lsp})
target_link_libraries(ednLsp PUBLIC ednCore cursiv::lsp)
add_executable(edn_lsp edn/lsp/main.cpp)
target_link_libraries(edn_lsp PRIVATE ednLsp)

cursiv_add_benchmark(edn SOURCES edn/bench/main.cpp)

cursiv_package_language(edn)
