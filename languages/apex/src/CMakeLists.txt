# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `apexGenerated` (the compiled library) and `apex_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID apex
    GRAMMAR    apex/core/ApexGrammar.hpp
    NAMESPACE  apex::Apex
    EXTENSIONS .cls
    OUT        apex/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apex/core/*.cpp)
add_library(apexCore ${_core})
target_link_libraries(apexCore PUBLIC apexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apex/cli/*.cpp)
add_executable(apex_cli ${_cli})
target_link_libraries(apex_cli
                      PRIVATE apexCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apex/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/apex/lsp/main.cpp)
add_library(apexLsp ${_lsp})
target_link_libraries(apexLsp PUBLIC apexCore cursiv::lsp)
add_executable(apex_lsp apex/lsp/main.cpp)
target_link_libraries(apex_lsp PRIVATE apexLsp)

cursiv_add_benchmark(apex SOURCES apex/bench/main.cpp)

cursiv_package_language(apex)
