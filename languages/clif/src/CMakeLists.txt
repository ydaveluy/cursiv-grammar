# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `clifGenerated` (the compiled library) and `clif_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID clif
    GRAMMAR    clif/core/ClifGrammar.hpp
    NAMESPACE  clif::Clif
    EXTENSIONS .clif
    OUT        clif/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clif/core/*.cpp)
add_library(clifCore ${_core})
target_link_libraries(clifCore PUBLIC clifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clif/cli/*.cpp)
add_executable(clif_cli ${_cli})
target_link_libraries(clif_cli
                      PRIVATE clifCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clif/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clif/lsp/main.cpp)
add_library(clifLsp ${_lsp})
target_link_libraries(clifLsp PUBLIC clifCore cursiv::lsp)
add_executable(clif_lsp clif/lsp/main.cpp)
target_link_libraries(clif_lsp PRIVATE clifLsp)

cursiv_add_benchmark(clif SOURCES clif/bench/main.cpp)

cursiv_package_language(clif)
