# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `d2Generated` (the compiled library) and `d2_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID d2
    GRAMMAR    d2/core/D2Grammar.hpp
    NAMESPACE  d2::D2
    EXTENSIONS .d2
    OUT        d2/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/d2/core/*.cpp)
add_library(d2Core ${_core})
target_link_libraries(d2Core PUBLIC d2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/d2/cli/*.cpp)
add_executable(d2_cli ${_cli})
target_link_libraries(d2_cli
                      PRIVATE d2Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/d2/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/d2/lsp/main.cpp)
add_library(d2Lsp ${_lsp})
target_link_libraries(d2Lsp PUBLIC d2Core cursiv::lsp)
add_executable(d2_lsp d2/lsp/main.cpp)
target_link_libraries(d2_lsp PRIVATE d2Lsp)

cursiv_add_benchmark(d2 SOURCES d2/bench/main.cpp)

cursiv_package_language(d2)
