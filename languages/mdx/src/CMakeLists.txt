# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mdxGenerated` (the compiled library) and `mdx_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID mdx
    GRAMMAR    mdx/core/MdxGrammar.hpp
    NAMESPACE  mdx::Mdx
    EXTENSIONS .mdx
    OUT        mdx/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mdx/core/*.cpp)
add_library(mdxCore ${_core})
target_link_libraries(mdxCore PUBLIC mdxGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mdx/cli/*.cpp)
add_executable(mdx_cli ${_cli})
target_link_libraries(mdx_cli
                      PRIVATE mdxCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mdx/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mdx/lsp/main.cpp)
add_library(mdxLsp ${_lsp})
target_link_libraries(mdxLsp PUBLIC mdxCore cursiv::lsp)
add_executable(mdx_lsp mdx/lsp/main.cpp)
target_link_libraries(mdx_lsp PRIVATE mdxLsp)

cursiv_add_benchmark(mdx SOURCES mdx/bench/main.cpp)

cursiv_package_language(mdx)
