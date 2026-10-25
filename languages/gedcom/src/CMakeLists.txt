# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gedcomGenerated` (the compiled library) and `gedcom_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gedcom
    GRAMMAR    gedcom/core/GedcomGrammar.hpp
    NAMESPACE  gedcom::Gedcom
    EXTENSIONS .ged
    OUT        gedcom/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/core/*.cpp)
add_library(gedcomCore ${_core})
target_link_libraries(gedcomCore PUBLIC gedcomGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/cli/*.cpp)
add_executable(gedcom_cli ${_cli})
target_link_libraries(gedcom_cli
                      PRIVATE gedcomCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/lsp/main.cpp)
add_library(gedcomLsp ${_lsp})
target_link_libraries(gedcomLsp PUBLIC gedcomCore cursiv::lsp)
add_executable(gedcom_lsp gedcom/lsp/main.cpp)
target_link_libraries(gedcom_lsp PRIVATE gedcomLsp)

cursiv_add_benchmark(gedcom SOURCES gedcom/bench/main.cpp)

cursiv_package_language(gedcom)
