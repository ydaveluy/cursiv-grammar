# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `alloyGenerated` (the compiled library) and `alloy_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID alloy
    GRAMMAR    alloy/core/AlloyGrammar.hpp
    NAMESPACE  alloy::Alloy
    EXTENSIONS .als
    OUT        alloy/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alloy/core/*.cpp)
add_library(alloyCore ${_core})
target_link_libraries(alloyCore PUBLIC alloyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alloy/cli/*.cpp)
add_executable(alloy_cli ${_cli})
target_link_libraries(alloy_cli
                      PRIVATE alloyCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alloy/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alloy/lsp/main.cpp)
add_library(alloyLsp ${_lsp})
target_link_libraries(alloyLsp PUBLIC alloyCore cursiv::lsp)
add_executable(alloy_lsp alloy/lsp/main.cpp)
target_link_libraries(alloy_lsp PRIVATE alloyLsp)

cursiv_add_benchmark(alloy SOURCES alloy/bench/main.cpp)

cursiv_package_language(alloy)
