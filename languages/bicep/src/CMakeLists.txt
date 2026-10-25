# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bicepGenerated` (the compiled library) and `bicep_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bicep
    GRAMMAR    bicep/core/BicepGrammar.hpp
    NAMESPACE  bicep::Bicep
    EXTENSIONS .bicep
    OUT        bicep/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bicep/core/*.cpp)
add_library(bicepCore ${_core})
target_link_libraries(bicepCore PUBLIC bicepGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bicep/cli/*.cpp)
add_executable(bicep_cli ${_cli})
target_link_libraries(bicep_cli
                      PRIVATE bicepCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bicep/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bicep/lsp/main.cpp)
add_library(bicepLsp ${_lsp})
target_link_libraries(bicepLsp PUBLIC bicepCore cursiv::lsp)
add_executable(bicep_lsp bicep/lsp/main.cpp)
target_link_libraries(bicep_lsp PRIVATE bicepLsp)

cursiv_add_benchmark(bicep SOURCES bicep/bench/main.cpp)

cursiv_package_language(bicep)
