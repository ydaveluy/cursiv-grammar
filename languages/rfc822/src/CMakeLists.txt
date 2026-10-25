# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rfc822Generated` (the compiled library) and `rfc822_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rfc822
    GRAMMAR    rfc822/core/Rfc822Grammar.hpp
    NAMESPACE  rfc822::Rfc822
    EXTENSIONS .rfc822
    OUT        rfc822/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/core/*.cpp)
add_library(rfc822Core ${_core})
target_link_libraries(rfc822Core PUBLIC rfc822Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/cli/*.cpp)
add_executable(rfc822_cli ${_cli})
target_link_libraries(rfc822_cli
                      PRIVATE rfc822Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/lsp/main.cpp)
add_library(rfc822Lsp ${_lsp})
target_link_libraries(rfc822Lsp PUBLIC rfc822Core cursiv::lsp)
add_executable(rfc822_lsp rfc822/lsp/main.cpp)
target_link_libraries(rfc822_lsp PRIVATE rfc822Lsp)

cursiv_add_benchmark(rfc822 SOURCES rfc822/bench/main.cpp)

cursiv_package_language(rfc822)
