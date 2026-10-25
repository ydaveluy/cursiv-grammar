# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sgfGenerated` (the compiled library) and `sgf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sgf
    GRAMMAR    sgf/core/SgfGrammar.hpp
    NAMESPACE  sgf::Sgf
    EXTENSIONS .sgf
    OUT        sgf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sgf/core/*.cpp)
add_library(sgfCore ${_core})
target_link_libraries(sgfCore PUBLIC sgfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sgf/cli/*.cpp)
add_executable(sgf_cli ${_cli})
target_link_libraries(sgf_cli
                      PRIVATE sgfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sgf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sgf/lsp/main.cpp)
add_library(sgfLsp ${_lsp})
target_link_libraries(sgfLsp PUBLIC sgfCore cursiv::lsp)
add_executable(sgf_lsp sgf/lsp/main.cpp)
target_link_libraries(sgf_lsp PRIVATE sgfLsp)

cursiv_add_benchmark(sgf SOURCES sgf/bench/main.cpp)

cursiv_package_language(sgf)
