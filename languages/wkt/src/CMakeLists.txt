# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `wktGenerated` (the compiled library) and `wkt_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wkt
    GRAMMAR    wkt/core/WktGrammar.hpp
    NAMESPACE  wkt::Wkt
    EXTENSIONS .wkt
    OUT        wkt/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wkt/core/*.cpp)
add_library(wktCore ${_core})
target_link_libraries(wktCore PUBLIC wktGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wkt/cli/*.cpp)
add_executable(wkt_cli ${_cli})
target_link_libraries(wkt_cli
                      PRIVATE wktCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wkt/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wkt/lsp/main.cpp)
add_library(wktLsp ${_lsp})
target_link_libraries(wktLsp PUBLIC wktCore cursiv::lsp)
add_executable(wkt_lsp wkt/lsp/main.cpp)
target_link_libraries(wkt_lsp PRIVATE wktLsp)

cursiv_add_benchmark(wkt SOURCES wkt/bench/main.cpp)

cursiv_package_language(wkt)
