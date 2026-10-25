# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `starGenerated` (the compiled library) and `star_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID star
    GRAMMAR    star/core/StarGrammar.hpp
    NAMESPACE  star::Star
    EXTENSIONS .star
    OUT        star/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/star/core/*.cpp)
add_library(starCore ${_core})
target_link_libraries(starCore PUBLIC starGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/star/cli/*.cpp)
add_executable(star_cli ${_cli})
target_link_libraries(star_cli
                      PRIVATE starCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/star/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/star/lsp/main.cpp)
add_library(starLsp ${_lsp})
target_link_libraries(starLsp PUBLIC starCore cursiv::lsp)
add_executable(star_lsp star/lsp/main.cpp)
target_link_libraries(star_lsp PRIVATE starLsp)

cursiv_add_benchmark(star SOURCES star/bench/main.cpp)

cursiv_package_language(star)
