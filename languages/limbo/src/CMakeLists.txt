# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `limboGenerated` (the compiled library) and `limbo_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID limbo
    GRAMMAR    limbo/core/LimboGrammar.hpp
    NAMESPACE  limbo::Limbo
    EXTENSIONS .b
    OUT        limbo/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/limbo/core/*.cpp)
add_library(limboCore ${_core})
target_link_libraries(limboCore PUBLIC limboGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/limbo/cli/*.cpp)
add_executable(limbo_cli ${_cli})
target_link_libraries(limbo_cli
                      PRIVATE limboCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/limbo/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/limbo/lsp/main.cpp)
add_library(limboLsp ${_lsp})
target_link_libraries(limboLsp PUBLIC limboCore cursiv::lsp)
add_executable(limbo_lsp limbo/lsp/main.cpp)
target_link_libraries(limbo_lsp PRIVATE limboLsp)

cursiv_add_benchmark(limbo SOURCES limbo/bench/main.cpp)

cursiv_package_language(limbo)
