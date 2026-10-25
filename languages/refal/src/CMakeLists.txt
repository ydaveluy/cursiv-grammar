# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `refalGenerated` (the compiled library) and `refal_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID refal
    GRAMMAR    refal/core/RefalGrammar.hpp
    NAMESPACE  refal::Refal
    EXTENSIONS .ref
    OUT        refal/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/refal/core/*.cpp)
add_library(refalCore ${_core})
target_link_libraries(refalCore PUBLIC refalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/refal/cli/*.cpp)
add_executable(refal_cli ${_cli})
target_link_libraries(refal_cli
                      PRIVATE refalCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/refal/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/refal/lsp/main.cpp)
add_library(refalLsp ${_lsp})
target_link_libraries(refalLsp PUBLIC refalCore cursiv::lsp)
add_executable(refal_lsp refal/lsp/main.cpp)
target_link_libraries(refal_lsp PRIVATE refalLsp)

cursiv_add_benchmark(refal SOURCES refal/bench/main.cpp)

cursiv_package_language(refal)
