# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `graphemesGenerated` (the compiled library) and `graphemes_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID graphemes
    GRAMMAR    graphemes/core/GraphemesGrammar.hpp
    NAMESPACE  graphemes::Graphemes
    EXTENSIONS .graphemes
    OUT        graphemes/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/core/*.cpp)
add_library(graphemesCore ${_core})
target_link_libraries(graphemesCore PUBLIC graphemesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/cli/*.cpp)
add_executable(graphemes_cli ${_cli})
target_link_libraries(graphemes_cli
                      PRIVATE graphemesCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/lsp/main.cpp)
add_library(graphemesLsp ${_lsp})
target_link_libraries(graphemesLsp PUBLIC graphemesCore cursiv::lsp)
add_executable(graphemes_lsp graphemes/lsp/main.cpp)
target_link_libraries(graphemes_lsp PRIVATE graphemesLsp)

cursiv_add_benchmark(graphemes SOURCES graphemes/bench/main.cpp)

cursiv_package_language(graphemes)
