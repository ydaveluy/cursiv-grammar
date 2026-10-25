# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `powerbuilderGenerated` (the compiled library) and `powerbuilder_generate`
# (writes the sources without compiling them).
# The seven PowerScript source-export kinds this grammar reads (window,
# user object, menu, application, function, structure, query); .srd
# (DataWindow) is a different grammar, ported separately as powerbuilderdw.
cursiv_generate_language(ID powerbuilder
    GRAMMAR    powerbuilder/core/PowerbuilderGrammar.hpp
    NAMESPACE  powerbuilder::Powerbuilder
    EXTENSIONS .srw .sru .srm .sra .srf .srs .srq
    OUT        powerbuilder/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/core/*.cpp)
add_library(powerbuilderCore ${_core})
target_link_libraries(powerbuilderCore PUBLIC powerbuilderGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/cli/*.cpp)
add_executable(powerbuilder_cli ${_cli})
target_link_libraries(powerbuilder_cli
                      PRIVATE powerbuilderCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/lsp/main.cpp)
add_library(powerbuilderLsp ${_lsp})
target_link_libraries(powerbuilderLsp PUBLIC powerbuilderCore cursiv::lsp)
add_executable(powerbuilder_lsp powerbuilder/lsp/main.cpp)
target_link_libraries(powerbuilder_lsp PRIVATE powerbuilderLsp)

cursiv_add_benchmark(powerbuilder SOURCES powerbuilder/bench/main.cpp)

cursiv_package_language(powerbuilder)
