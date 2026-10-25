# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `plyGenerated` (the compiled library) and `ply_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ply
    GRAMMAR    ply/core/PlyGrammar.hpp
    NAMESPACE  ply::Ply
    EXTENSIONS .ply
    OUT        ply/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ply/core/*.cpp)
add_library(plyCore ${_core})
target_link_libraries(plyCore PUBLIC plyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ply/cli/*.cpp)
add_executable(ply_cli ${_cli})
target_link_libraries(ply_cli
                      PRIVATE plyCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ply/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ply/lsp/main.cpp)
add_library(plyLsp ${_lsp})
target_link_libraries(plyLsp PUBLIC plyCore cursiv::lsp)
add_executable(ply_lsp ply/lsp/main.cpp)
target_link_libraries(ply_lsp PRIVATE plyLsp)

cursiv_add_benchmark(ply SOURCES ply/bench/main.cpp)

cursiv_package_language(ply)
