# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `plucidGenerated` (the compiled library) and `plucid_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID plucid
    GRAMMAR    plucid/core/PlucidGrammar.hpp
    NAMESPACE  plucid::Plucid
    EXTENSIONS .plu
    OUT        plucid/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plucid/core/*.cpp)
add_library(plucidCore ${_core})
target_link_libraries(plucidCore PUBLIC plucidGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plucid/cli/*.cpp)
add_executable(plucid_cli ${_cli})
target_link_libraries(plucid_cli
                      PRIVATE plucidCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plucid/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/plucid/lsp/main.cpp)
add_library(plucidLsp ${_lsp})
target_link_libraries(plucidLsp PUBLIC plucidCore cursiv::lsp)
add_executable(plucid_lsp plucid/lsp/main.cpp)
target_link_libraries(plucid_lsp PRIVATE plucidLsp)

cursiv_add_benchmark(plucid SOURCES plucid/bench/main.cpp)

cursiv_package_language(plucid)
