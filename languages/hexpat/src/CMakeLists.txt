# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `hexpatGenerated` (the compiled library) and `hexpat_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID hexpat
    GRAMMAR    hexpat/core/HexpatGrammar.hpp
    NAMESPACE  hexpat::Hexpat
    EXTENSIONS .hexpat
    OUT        hexpat/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/core/*.cpp)
add_library(hexpatCore ${_core})
target_link_libraries(hexpatCore PUBLIC hexpatGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/cli/*.cpp)
add_executable(hexpat_cli ${_cli})
target_link_libraries(hexpat_cli
                      PRIVATE hexpatCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/lsp/main.cpp)
add_library(hexpatLsp ${_lsp})
target_link_libraries(hexpatLsp PUBLIC hexpatCore cursiv::lsp)
add_executable(hexpat_lsp hexpat/lsp/main.cpp)
target_link_libraries(hexpat_lsp PRIVATE hexpatLsp)

cursiv_add_benchmark(hexpat SOURCES hexpat/bench/main.cpp)

cursiv_package_language(hexpat)
