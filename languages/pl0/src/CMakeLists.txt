# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pl0Generated` (the compiled library) and `pl0_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pl0
    GRAMMAR    pl0/core/Pl0Grammar.hpp
    NAMESPACE  pl0::Pl0
    EXTENSIONS .pl0
    OUT        pl0/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pl0/core/*.cpp)
add_library(pl0Core ${_core})
target_link_libraries(pl0Core PUBLIC pl0Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pl0/cli/*.cpp)
add_executable(pl0_cli ${_cli})
target_link_libraries(pl0_cli
                      PRIVATE pl0Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pl0/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pl0/lsp/main.cpp)
add_library(pl0Lsp ${_lsp})
target_link_libraries(pl0Lsp PUBLIC pl0Core cursiv::lsp)
add_executable(pl0_lsp pl0/lsp/main.cpp)
target_link_libraries(pl0_lsp PRIVATE pl0Lsp)

cursiv_add_benchmark(pl0 SOURCES pl0/bench/main.cpp)

cursiv_package_language(pl0)
