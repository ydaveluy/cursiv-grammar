# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tracGenerated` (the compiled library) and `trac_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID trac
    GRAMMAR    trac/core/TracGrammar.hpp
    NAMESPACE  trac::Trac
    EXTENSIONS .trac
    OUT        trac/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trac/core/*.cpp)
add_library(tracCore ${_core})
target_link_libraries(tracCore PUBLIC tracGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trac/cli/*.cpp)
add_executable(trac_cli ${_cli})
target_link_libraries(trac_cli
                      PRIVATE tracCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trac/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/trac/lsp/main.cpp)
add_library(tracLsp ${_lsp})
target_link_libraries(tracLsp PUBLIC tracCore cursiv::lsp)
add_executable(trac_lsp trac/lsp/main.cpp)
target_link_libraries(trac_lsp PRIVATE tracLsp)

cursiv_add_benchmark(trac SOURCES trac/bench/main.cpp)

cursiv_package_language(trac)
