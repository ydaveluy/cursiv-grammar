# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `graphstreamdgsGenerated` (the compiled library) and `graphstreamdgs_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID graphstreamdgs
    GRAMMAR    graphstreamdgs/core/GraphstreamDgsGrammar.hpp
    NAMESPACE  graphstreamdgs::GraphstreamDgs
    EXTENSIONS .dgs
    OUT        graphstreamdgs/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/core/*.cpp)
add_library(graphstreamdgsCore ${_core})
target_link_libraries(graphstreamdgsCore PUBLIC graphstreamdgsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/cli/*.cpp)
add_executable(graphstreamdgs_cli ${_cli})
target_link_libraries(graphstreamdgs_cli
                      PRIVATE graphstreamdgsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/lsp/main.cpp)
add_library(graphstreamdgsLsp ${_lsp})
target_link_libraries(graphstreamdgsLsp PUBLIC graphstreamdgsCore cursiv::lsp)
add_executable(graphstreamdgs_lsp graphstreamdgs/lsp/main.cpp)
target_link_libraries(graphstreamdgs_lsp PRIVATE graphstreamdgsLsp)

cursiv_add_benchmark(graphstreamdgs SOURCES graphstreamdgs/bench/main.cpp)

cursiv_package_language(graphstreamdgs)
