# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mpsGenerated` (the compiled library) and `mps_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID mps
    GRAMMAR    mps/core/MpsGrammar.hpp
    NAMESPACE  mps::Mps
    EXTENSIONS .mps
    OUT        mps/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mps/core/*.cpp)
add_library(mpsCore ${_core})
target_link_libraries(mpsCore PUBLIC mpsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mps/cli/*.cpp)
add_executable(mps_cli ${_cli})
target_link_libraries(mps_cli
                      PRIVATE mpsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mps/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mps/lsp/main.cpp)
add_library(mpsLsp ${_lsp})
target_link_libraries(mpsLsp PUBLIC mpsCore cursiv::lsp)
add_executable(mps_lsp mps/lsp/main.cpp)
target_link_libraries(mps_lsp PRIVATE mpsLsp)

cursiv_add_benchmark(mps SOURCES mps/bench/main.cpp)

cursiv_package_language(mps)
