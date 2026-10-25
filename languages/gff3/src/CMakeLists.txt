# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gff3Generated` (the compiled library) and `gff3_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gff3
    GRAMMAR    gff3/core/Gff3Grammar.hpp
    NAMESPACE  gff3::Gff3
    EXTENSIONS .gff
    OUT        gff3/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gff3/core/*.cpp)
add_library(gff3Core ${_core})
target_link_libraries(gff3Core PUBLIC gff3Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gff3/cli/*.cpp)
add_executable(gff3_cli ${_cli})
target_link_libraries(gff3_cli
                      PRIVATE gff3Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gff3/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gff3/lsp/main.cpp)
add_library(gff3Lsp ${_lsp})
target_link_libraries(gff3Lsp PUBLIC gff3Core cursiv::lsp)
add_executable(gff3_lsp gff3/lsp/main.cpp)
target_link_libraries(gff3_lsp PRIVATE gff3Lsp)

cursiv_add_benchmark(gff3 SOURCES gff3/bench/main.cpp)

cursiv_package_language(gff3)
