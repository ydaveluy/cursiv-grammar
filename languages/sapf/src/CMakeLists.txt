# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sapfGenerated` (the compiled library) and `sapf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sapf
    GRAMMAR    sapf/core/SapfGrammar.hpp
    NAMESPACE  sapf::Sapf
    EXTENSIONS .sapf
    OUT        sapf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sapf/core/*.cpp)
add_library(sapfCore ${_core})
target_link_libraries(sapfCore PUBLIC sapfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sapf/cli/*.cpp)
add_executable(sapf_cli ${_cli})
target_link_libraries(sapf_cli
                      PRIVATE sapfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sapf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sapf/lsp/main.cpp)
add_library(sapfLsp ${_lsp})
target_link_libraries(sapfLsp PUBLIC sapfCore cursiv::lsp)
add_executable(sapf_lsp sapf/lsp/main.cpp)
target_link_libraries(sapf_lsp PRIVATE sapfLsp)

cursiv_add_benchmark(sapf SOURCES sapf/bench/main.cpp)

cursiv_package_language(sapf)
