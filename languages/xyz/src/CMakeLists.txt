# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `xyzGenerated` (the compiled library) and `xyz_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID xyz
    GRAMMAR    xyz/core/XyzGrammar.hpp
    NAMESPACE  xyz::Xyz
    EXTENSIONS .xyz
    OUT        xyz/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xyz/core/*.cpp)
add_library(xyzCore ${_core})
target_link_libraries(xyzCore PUBLIC xyzGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xyz/cli/*.cpp)
add_executable(xyz_cli ${_cli})
target_link_libraries(xyz_cli
                      PRIVATE xyzCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xyz/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xyz/lsp/main.cpp)
add_library(xyzLsp ${_lsp})
target_link_libraries(xyzLsp PUBLIC xyzCore cursiv::lsp)
add_executable(xyz_lsp xyz/lsp/main.cpp)
target_link_libraries(xyz_lsp PRIVATE xyzLsp)

cursiv_add_benchmark(xyz SOURCES xyz/bench/main.cpp)

cursiv_package_language(xyz)
