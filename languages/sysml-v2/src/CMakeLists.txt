# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sysmlv2Generated` (the compiled library) and `sysmlv2_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sysmlv2
    GRAMMAR    sysmlv2/core/SysmlV2Grammar.hpp
    NAMESPACE  sysmlv2::SysmlV2
    EXTENSIONS .sysml
    OUT        sysmlv2/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/core/*.cpp)
add_library(sysmlv2Core ${_core})
target_link_libraries(sysmlv2Core PUBLIC sysmlv2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/cli/*.cpp)
add_executable(sysmlv2_cli ${_cli})
target_link_libraries(sysmlv2_cli
                      PRIVATE sysmlv2Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/lsp/main.cpp)
add_library(sysmlv2Lsp ${_lsp})
target_link_libraries(sysmlv2Lsp PUBLIC sysmlv2Core cursiv::lsp)
add_executable(sysmlv2_lsp sysmlv2/lsp/main.cpp)
target_link_libraries(sysmlv2_lsp PRIVATE sysmlv2Lsp)

cursiv_add_benchmark(sysmlv2 SOURCES sysmlv2/bench/main.cpp)

cursiv_package_language(sysmlv2)
