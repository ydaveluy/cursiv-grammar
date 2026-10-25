# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `smtlibv2Generated` (the compiled library) and `smtlibv2_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID smtlibv2
    GRAMMAR    smtlibv2/core/Smtlibv2Grammar.hpp
    NAMESPACE  smtlibv2::Smtlibv2
    EXTENSIONS .smt2
    OUT        smtlibv2/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/core/*.cpp)
add_library(smtlibv2Core ${_core})
target_link_libraries(smtlibv2Core PUBLIC smtlibv2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/cli/*.cpp)
add_executable(smtlibv2_cli ${_cli})
target_link_libraries(smtlibv2_cli
                      PRIVATE smtlibv2Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/lsp/main.cpp)
add_library(smtlibv2Lsp ${_lsp})
target_link_libraries(smtlibv2Lsp PUBLIC smtlibv2Core cursiv::lsp)
add_executable(smtlibv2_lsp smtlibv2/lsp/main.cpp)
target_link_libraries(smtlibv2_lsp PRIVATE smtlibv2Lsp)

cursiv_add_benchmark(smtlibv2 SOURCES smtlibv2/bench/main.cpp)

cursiv_package_language(smtlibv2)
