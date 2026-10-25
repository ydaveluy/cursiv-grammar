# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `modula2pim4Generated` (the compiled library) and `modula2pim4_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID modula2pim4
    GRAMMAR    modula2pim4/core/Modula2pim4Grammar.hpp
    NAMESPACE  modula2pim4::Modula2pim4
    EXTENSIONS .mod
    OUT        modula2pim4/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/core/*.cpp)
add_library(modula2pim4Core ${_core})
target_link_libraries(modula2pim4Core PUBLIC modula2pim4Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/cli/*.cpp)
add_executable(modula2pim4_cli ${_cli})
target_link_libraries(modula2pim4_cli
                      PRIVATE modula2pim4Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/lsp/main.cpp)
add_library(modula2pim4Lsp ${_lsp})
target_link_libraries(modula2pim4Lsp PUBLIC modula2pim4Core cursiv::lsp)
add_executable(modula2pim4_lsp modula2pim4/lsp/main.cpp)
target_link_libraries(modula2pim4_lsp PRIVATE modula2pim4Lsp)

cursiv_add_benchmark(modula2pim4 SOURCES modula2pim4/bench/main.cpp)

cursiv_package_language(modula2pim4)
