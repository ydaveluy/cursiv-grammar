# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pcreGenerated` (the compiled library) and `pcre_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pcre
    GRAMMAR    pcre/core/PcreGrammar.hpp
    NAMESPACE  pcre::Pcre
    EXTENSIONS .pcre
    OUT        pcre/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pcre/core/*.cpp)
add_library(pcreCore ${_core})
target_link_libraries(pcreCore PUBLIC pcreGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pcre/cli/*.cpp)
add_executable(pcre_cli ${_cli})
target_link_libraries(pcre_cli
                      PRIVATE pcreCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pcre/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pcre/lsp/main.cpp)
add_library(pcreLsp ${_lsp})
target_link_libraries(pcreLsp PUBLIC pcreCore cursiv::lsp)
add_executable(pcre_lsp pcre/lsp/main.cpp)
target_link_libraries(pcre_lsp PRIVATE pcreLsp)

cursiv_add_benchmark(pcre SOURCES pcre/bench/main.cpp)

cursiv_package_language(pcre)
