# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `corundumGenerated` (the compiled library) and `corundum_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID corundum
    GRAMMAR    corundum/core/CorundumGrammar.hpp
    NAMESPACE  corundum::Corundum
    EXTENSIONS .rb
    OUT        corundum/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/corundum/core/*.cpp)
add_library(corundumCore ${_core})
target_link_libraries(corundumCore PUBLIC corundumGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/corundum/cli/*.cpp)
add_executable(corundum_cli ${_cli})
target_link_libraries(corundum_cli
                      PRIVATE corundumCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/corundum/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/corundum/lsp/main.cpp)
add_library(corundumLsp ${_lsp})
target_link_libraries(corundumLsp PUBLIC corundumCore cursiv::lsp)
add_executable(corundum_lsp corundum/lsp/main.cpp)
target_link_libraries(corundum_lsp PRIVATE corundumLsp)

cursiv_add_benchmark(corundum SOURCES corundum/bench/main.cpp)

cursiv_package_language(corundum)
