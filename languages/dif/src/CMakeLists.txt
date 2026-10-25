# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `difGenerated` (the compiled library) and `dif_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID dif
    GRAMMAR    dif/core/DifGrammar.hpp
    NAMESPACE  dif::Dif
    EXTENSIONS .dif
    OUT        dif/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dif/core/*.cpp)
add_library(difCore ${_core})
target_link_libraries(difCore PUBLIC difGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dif/cli/*.cpp)
add_executable(dif_cli ${_cli})
target_link_libraries(dif_cli
                      PRIVATE difCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dif/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dif/lsp/main.cpp)
add_library(difLsp ${_lsp})
target_link_libraries(difLsp PUBLIC difCore cursiv::lsp)
add_executable(dif_lsp dif/lsp/main.cpp)
target_link_libraries(dif_lsp PRIVATE difLsp)

cursiv_add_benchmark(dif SOURCES dif/bench/main.cpp)

cursiv_package_language(dif)
