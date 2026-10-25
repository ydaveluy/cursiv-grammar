# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `fdo91Generated` (the compiled library) and `fdo91_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fdo91
    GRAMMAR    fdo91/core/Fdo91Grammar.hpp
    NAMESPACE  fdo91::Fdo91
    EXTENSIONS .fdo91
    OUT        fdo91/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/core/*.cpp)
add_library(fdo91Core ${_core})
target_link_libraries(fdo91Core PUBLIC fdo91Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/cli/*.cpp)
add_executable(fdo91_cli ${_cli})
target_link_libraries(fdo91_cli
                      PRIVATE fdo91Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/lsp/main.cpp)
add_library(fdo91Lsp ${_lsp})
target_link_libraries(fdo91Lsp PUBLIC fdo91Core cursiv::lsp)
add_executable(fdo91_lsp fdo91/lsp/main.cpp)
target_link_libraries(fdo91_lsp PRIVATE fdo91Lsp)

cursiv_add_benchmark(fdo91 SOURCES fdo91/bench/main.cpp)

cursiv_package_language(fdo91)
