# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `wktcrsv1Generated` (the compiled library) and `wktcrsv1_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wktcrsv1
    GRAMMAR    wktcrsv1/core/WktCrsV1Grammar.hpp
    NAMESPACE  wktcrsv1::WktCrsV1
    EXTENSIONS .wktcrsv1
    OUT        wktcrsv1/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/core/*.cpp)
add_library(wktcrsv1Core ${_core})
target_link_libraries(wktcrsv1Core PUBLIC wktcrsv1Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/cli/*.cpp)
add_executable(wktcrsv1_cli ${_cli})
target_link_libraries(wktcrsv1_cli
                      PRIVATE wktcrsv1Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/lsp/main.cpp)
add_library(wktcrsv1Lsp ${_lsp})
target_link_libraries(wktcrsv1Lsp PUBLIC wktcrsv1Core cursiv::lsp)
add_executable(wktcrsv1_lsp wktcrsv1/lsp/main.cpp)
target_link_libraries(wktcrsv1_lsp PRIVATE wktcrsv1Lsp)

cursiv_add_benchmark(wktcrsv1 SOURCES wktcrsv1/bench/main.cpp)

cursiv_package_language(wktcrsv1)
