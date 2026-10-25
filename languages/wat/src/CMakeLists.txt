# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `watGenerated` (the compiled library) and `wat_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wat
    GRAMMAR    wat/core/WatGrammar.hpp
    NAMESPACE  wat::Wat
    EXTENSIONS .wat
    OUT        wat/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wat/core/*.cpp)
add_library(watCore ${_core})
target_link_libraries(watCore PUBLIC watGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wat/cli/*.cpp)
add_executable(wat_cli ${_cli})
target_link_libraries(wat_cli
                      PRIVATE watCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wat/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wat/lsp/main.cpp)
add_library(watLsp ${_lsp})
target_link_libraries(watLsp PUBLIC watCore cursiv::lsp)
add_executable(wat_lsp wat/lsp/main.cpp)
target_link_libraries(wat_lsp PRIVATE watLsp)

cursiv_add_benchmark(wat SOURCES wat/bench/main.cpp)

cursiv_package_language(wat)
