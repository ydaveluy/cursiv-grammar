# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rcsGenerated` (the compiled library) and `rcs_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rcs
    GRAMMAR    rcs/core/RcsGrammar.hpp
    NAMESPACE  rcs::Rcs
    EXTENSIONS .rcs
    OUT        rcs/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rcs/core/*.cpp)
add_library(rcsCore ${_core})
target_link_libraries(rcsCore PUBLIC rcsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rcs/cli/*.cpp)
add_executable(rcs_cli ${_cli})
target_link_libraries(rcs_cli
                      PRIVATE rcsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rcs/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rcs/lsp/main.cpp)
add_library(rcsLsp ${_lsp})
target_link_libraries(rcsLsp PUBLIC rcsCore cursiv::lsp)
add_executable(rcs_lsp rcs/lsp/main.cpp)
target_link_libraries(rcs_lsp PRIVATE rcsLsp)

cursiv_add_benchmark(rcs SOURCES rcs/bench/main.cpp)

cursiv_package_language(rcs)
