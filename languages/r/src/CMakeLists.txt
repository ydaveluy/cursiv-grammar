# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rGenerated` (the compiled library) and `r_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID r
    GRAMMAR    r/core/RGrammar.hpp
    NAMESPACE  r::R
    EXTENSIONS .R
    OUT        r/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/r/core/*.cpp)
add_library(rCore ${_core})
target_link_libraries(rCore PUBLIC rGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/r/cli/*.cpp)
add_executable(r_cli ${_cli})
target_link_libraries(r_cli
                      PRIVATE rCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/r/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/r/lsp/main.cpp)
add_library(rLsp ${_lsp})
target_link_libraries(rLsp PUBLIC rCore cursiv::lsp)
add_executable(r_lsp r/lsp/main.cpp)
target_link_libraries(r_lsp PRIVATE rLsp)

cursiv_add_benchmark(r SOURCES r/bench/main.cpp)

cursiv_package_language(r)
