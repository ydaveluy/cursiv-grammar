# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bdfGenerated` (the compiled library) and `bdf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bdf
    GRAMMAR    bdf/core/BdfGrammar.hpp
    NAMESPACE  bdf::Bdf
    EXTENSIONS .bdf
    OUT        bdf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bdf/core/*.cpp)
add_library(bdfCore ${_core})
target_link_libraries(bdfCore PUBLIC bdfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bdf/cli/*.cpp)
add_executable(bdf_cli ${_cli})
target_link_libraries(bdf_cli
                      PRIVATE bdfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bdf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bdf/lsp/main.cpp)
add_library(bdfLsp ${_lsp})
target_link_libraries(bdfLsp PUBLIC bdfCore cursiv::lsp)
add_executable(bdf_lsp bdf/lsp/main.cpp)
target_link_libraries(bdf_lsp PRIVATE bdfLsp)

cursiv_add_benchmark(bdf SOURCES bdf/bench/main.cpp)

cursiv_package_language(bdf)
