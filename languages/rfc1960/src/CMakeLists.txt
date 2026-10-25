# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rfc1960Generated` (the compiled library) and `rfc1960_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rfc1960
    GRAMMAR    rfc1960/core/Rfc1960Grammar.hpp
    NAMESPACE  rfc1960::Rfc1960
    EXTENSIONS .rfc1960
    OUT        rfc1960/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/core/*.cpp)
add_library(rfc1960Core ${_core})
target_link_libraries(rfc1960Core PUBLIC rfc1960Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/cli/*.cpp)
add_executable(rfc1960_cli ${_cli})
target_link_libraries(rfc1960_cli
                      PRIVATE rfc1960Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/lsp/main.cpp)
add_library(rfc1960Lsp ${_lsp})
target_link_libraries(rfc1960Lsp PUBLIC rfc1960Core cursiv::lsp)
add_executable(rfc1960_lsp rfc1960/lsp/main.cpp)
target_link_libraries(rfc1960_lsp PRIVATE rfc1960Lsp)

cursiv_add_benchmark(rfc1960 SOURCES rfc1960/bench/main.cpp)

cursiv_package_language(rfc1960)
