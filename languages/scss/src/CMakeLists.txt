# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `scssGenerated` (the compiled library) and `scss_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID scss
    GRAMMAR    scss/core/ScssGrammar.hpp
    NAMESPACE  scss::Scss
    EXTENSIONS .scss
    OUT        scss/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scss/core/*.cpp)
add_library(scssCore ${_core})
target_link_libraries(scssCore PUBLIC scssGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scss/cli/*.cpp)
add_executable(scss_cli ${_cli})
target_link_libraries(scss_cli
                      PRIVATE scssCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scss/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scss/lsp/main.cpp)
add_library(scssLsp ${_lsp})
target_link_libraries(scssLsp PUBLIC scssCore cursiv::lsp)
add_executable(scss_lsp scss/lsp/main.cpp)
target_link_libraries(scss_lsp PRIVATE scssLsp)

cursiv_add_benchmark(scss SOURCES scss/bench/main.cpp)

cursiv_package_language(scss)
