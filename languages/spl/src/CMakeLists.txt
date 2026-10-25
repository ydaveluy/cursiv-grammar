# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `splGenerated` (the compiled library) and `spl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID spl
    GRAMMAR    spl/core/SplGrammar.hpp
    NAMESPACE  spl::Spl
    EXTENSIONS .spl
    OUT        spl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spl/core/*.cpp)
add_library(splCore ${_core})
target_link_libraries(splCore PUBLIC splGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spl/cli/*.cpp)
add_executable(spl_cli ${_cli})
target_link_libraries(spl_cli
                      PRIVATE splCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/spl/lsp/main.cpp)
add_library(splLsp ${_lsp})
target_link_libraries(splLsp PUBLIC splCore cursiv::lsp)
add_executable(spl_lsp spl/lsp/main.cpp)
target_link_libraries(spl_lsp PRIVATE splLsp)

cursiv_add_benchmark(spl SOURCES spl/bench/main.cpp)

cursiv_package_language(spl)
