# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `fortranGenerated` (the compiled library) and `fortran_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fortran
    GRAMMAR    fortran/core/FortranGrammar.hpp
    NAMESPACE  fortran::Fortran
    EXTENSIONS .f90
    OUT        fortran/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fortran/core/*.cpp)
add_library(fortranCore ${_core})
target_link_libraries(fortranCore PUBLIC fortranGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fortran/cli/*.cpp)
add_executable(fortran_cli ${_cli})
target_link_libraries(fortran_cli
                      PRIVATE fortranCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fortran/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fortran/lsp/main.cpp)
add_library(fortranLsp ${_lsp})
target_link_libraries(fortranLsp PUBLIC fortranCore cursiv::lsp)
add_executable(fortran_lsp fortran/lsp/main.cpp)
target_link_libraries(fortran_lsp PRIVATE fortranLsp)

cursiv_add_benchmark(fortran SOURCES fortran/bench/main.cpp)

cursiv_package_language(fortran)
