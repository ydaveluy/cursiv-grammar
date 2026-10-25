# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pGenerated` (the compiled library) and `p_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID p
    GRAMMAR    p/core/PGrammar.hpp
    NAMESPACE  p::P
    EXTENSIONS .p
    OUT        p/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/p/core/*.cpp)
add_library(pCore ${_core})
target_link_libraries(pCore PUBLIC pGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/p/cli/*.cpp)
add_executable(p_cli ${_cli})
target_link_libraries(p_cli
                      PRIVATE pCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/p/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/p/lsp/main.cpp)
add_library(pLsp ${_lsp})
target_link_libraries(pLsp PUBLIC pCore cursiv::lsp)
add_executable(p_lsp p/lsp/main.cpp)
target_link_libraries(p_lsp PRIVATE pLsp)

cursiv_add_benchmark(p SOURCES p/bench/main.cpp)

cursiv_package_language(p)
