# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `vaxscanGenerated` (the compiled library) and `vaxscan_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID vaxscan
    GRAMMAR    vaxscan/core/VaxscanGrammar.hpp
    NAMESPACE  vaxscan::Vaxscan
    EXTENSIONS .scn
    OUT        vaxscan/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/core/*.cpp)
add_library(vaxscanCore ${_core})
target_link_libraries(vaxscanCore PUBLIC vaxscanGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/cli/*.cpp)
add_executable(vaxscan_cli ${_cli})
target_link_libraries(vaxscan_cli
                      PRIVATE vaxscanCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/lsp/main.cpp)
add_library(vaxscanLsp ${_lsp})
target_link_libraries(vaxscanLsp PUBLIC vaxscanCore cursiv::lsp)
add_executable(vaxscan_lsp vaxscan/lsp/main.cpp)
target_link_libraries(vaxscan_lsp PRIVATE vaxscanLsp)

cursiv_add_benchmark(vaxscan SOURCES vaxscan/bench/main.cpp)

cursiv_package_language(vaxscan)
