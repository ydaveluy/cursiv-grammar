# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mooGenerated` (the compiled library) and `moo_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID moo
    GRAMMAR    moo/core/MooGrammar.hpp
    NAMESPACE  moo::Moo
    EXTENSIONS .moo
    OUT        moo/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/moo/core/*.cpp)
add_library(mooCore ${_core})
target_link_libraries(mooCore PUBLIC mooGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/moo/cli/*.cpp)
add_executable(moo_cli ${_cli})
target_link_libraries(moo_cli
                      PRIVATE mooCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/moo/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/moo/lsp/main.cpp)
add_library(mooLsp ${_lsp})
target_link_libraries(mooLsp PUBLIC mooCore cursiv::lsp)
add_executable(moo_lsp moo/lsp/main.cpp)
target_link_libraries(moo_lsp PRIVATE mooLsp)

cursiv_add_benchmark(moo SOURCES moo/bench/main.cpp)

cursiv_package_language(moo)
