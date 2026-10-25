# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gvprGenerated` (the compiled library) and `gvpr_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gvpr
    GRAMMAR    gvpr/core/GvprGrammar.hpp
    NAMESPACE  gvpr::Gvpr
    EXTENSIONS .gvpr
    OUT        gvpr/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/core/*.cpp)
add_library(gvprCore ${_core})
target_link_libraries(gvprCore PUBLIC gvprGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/cli/*.cpp)
add_executable(gvpr_cli ${_cli})
target_link_libraries(gvpr_cli
                      PRIVATE gvprCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/lsp/main.cpp)
add_library(gvprLsp ${_lsp})
target_link_libraries(gvprLsp PUBLIC gvprCore cursiv::lsp)
add_executable(gvpr_lsp gvpr/lsp/main.cpp)
target_link_libraries(gvpr_lsp PRIVATE gvprLsp)

cursiv_add_benchmark(gvpr SOURCES gvpr/bench/main.cpp)

cursiv_package_language(gvpr)
