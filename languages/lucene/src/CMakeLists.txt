# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `luceneGenerated` (the compiled library) and `lucene_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lucene
    GRAMMAR    lucene/core/LuceneGrammar.hpp
    NAMESPACE  lucene::Lucene
    EXTENSIONS .lucene
    OUT        lucene/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lucene/core/*.cpp)
add_library(luceneCore ${_core})
target_link_libraries(luceneCore PUBLIC luceneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lucene/cli/*.cpp)
add_executable(lucene_cli ${_cli})
target_link_libraries(lucene_cli
                      PRIVATE luceneCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lucene/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lucene/lsp/main.cpp)
add_library(luceneLsp ${_lsp})
target_link_libraries(luceneLsp PUBLIC luceneCore cursiv::lsp)
add_executable(lucene_lsp lucene/lsp/main.cpp)
target_link_libraries(lucene_lsp PRIVATE luceneLsp)

cursiv_add_benchmark(lucene SOURCES lucene/bench/main.cpp)

cursiv_package_language(lucene)
