# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pbmGenerated` (the compiled library) and `pbm_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pbm
    GRAMMAR    pbm/core/PbmGrammar.hpp
    NAMESPACE  pbm::Pbm
    EXTENSIONS .pbm
    OUT        pbm/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pbm/core/*.cpp)
add_library(pbmCore ${_core})
target_link_libraries(pbmCore PUBLIC pbmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pbm/cli/*.cpp)
add_executable(pbm_cli ${_cli})
target_link_libraries(pbm_cli
                      PRIVATE pbmCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pbm/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pbm/lsp/main.cpp)
add_library(pbmLsp ${_lsp})
target_link_libraries(pbmLsp PUBLIC pbmCore cursiv::lsp)
add_executable(pbm_lsp pbm/lsp/main.cpp)
target_link_libraries(pbm_lsp PRIVATE pbmLsp)

cursiv_add_benchmark(pbm SOURCES pbm/bench/main.cpp)

cursiv_package_language(pbm)
