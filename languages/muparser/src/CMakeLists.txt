# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `muparserGenerated` (the compiled library) and `muparser_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID muparser
    GRAMMAR    muparser/core/MuparserGrammar.hpp
    NAMESPACE  muparser::Muparser
    EXTENSIONS .muparser
    OUT        muparser/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muparser/core/*.cpp)
add_library(muparserCore ${_core})
target_link_libraries(muparserCore PUBLIC muparserGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muparser/cli/*.cpp)
add_executable(muparser_cli ${_cli})
target_link_libraries(muparser_cli
                      PRIVATE muparserCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muparser/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/muparser/lsp/main.cpp)
add_library(muparserLsp ${_lsp})
target_link_libraries(muparserLsp PUBLIC muparserCore cursiv::lsp)
add_executable(muparser_lsp muparser/lsp/main.cpp)
target_link_libraries(muparser_lsp PRIVATE muparserLsp)

cursiv_add_benchmark(muparser SOURCES muparser/bench/main.cpp)

cursiv_package_language(muparser)
