# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `atermGenerated` (the compiled library) and `aterm_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID aterm
    GRAMMAR    aterm/core/AtermGrammar.hpp
    NAMESPACE  aterm::Aterm
    EXTENSIONS .aterm
    OUT        aterm/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aterm/core/*.cpp)
add_library(atermCore ${_core})
target_link_libraries(atermCore PUBLIC atermGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aterm/cli/*.cpp)
add_executable(aterm_cli ${_cli})
target_link_libraries(aterm_cli
                      PRIVATE atermCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aterm/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aterm/lsp/main.cpp)
add_library(atermLsp ${_lsp})
target_link_libraries(atermLsp PUBLIC atermCore cursiv::lsp)
add_executable(aterm_lsp aterm/lsp/main.cpp)
target_link_libraries(aterm_lsp PRIVATE atermLsp)

cursiv_add_benchmark(aterm SOURCES aterm/bench/main.cpp)

cursiv_package_language(aterm)
