# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gtinGenerated` (the compiled library) and `gtin_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gtin
    GRAMMAR    gtin/core/GtinGrammar.hpp
    NAMESPACE  gtin::Gtin
    EXTENSIONS .gtin
    OUT        gtin/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gtin/core/*.cpp)
add_library(gtinCore ${_core})
target_link_libraries(gtinCore PUBLIC gtinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gtin/cli/*.cpp)
add_executable(gtin_cli ${_cli})
target_link_libraries(gtin_cli
                      PRIVATE gtinCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gtin/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gtin/lsp/main.cpp)
add_library(gtinLsp ${_lsp})
target_link_libraries(gtinLsp PUBLIC gtinCore cursiv::lsp)
add_executable(gtin_lsp gtin/lsp/main.cpp)
target_link_libraries(gtin_lsp PRIVATE gtinLsp)

cursiv_add_benchmark(gtin SOURCES gtin/bench/main.cpp)

cursiv_package_language(gtin)
