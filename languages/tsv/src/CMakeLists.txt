# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tsvGenerated` (the compiled library) and `tsv_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tsv
    GRAMMAR    tsv/core/TsvGrammar.hpp
    NAMESPACE  tsv::Tsv
    EXTENSIONS .tsv
    OUT        tsv/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tsv/core/*.cpp)
add_library(tsvCore ${_core})
target_link_libraries(tsvCore PUBLIC tsvGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tsv/cli/*.cpp)
add_executable(tsv_cli ${_cli})
target_link_libraries(tsv_cli
                      PRIVATE tsvCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tsv/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tsv/lsp/main.cpp)
add_library(tsvLsp ${_lsp})
target_link_libraries(tsvLsp PUBLIC tsvCore cursiv::lsp)
add_executable(tsv_lsp tsv/lsp/main.cpp)
target_link_libraries(tsv_lsp PRIVATE tsvLsp)

cursiv_add_benchmark(tsv SOURCES tsv/bench/main.cpp)

cursiv_package_language(tsv)
