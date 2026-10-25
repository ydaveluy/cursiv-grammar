# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `infGenerated` (the compiled library) and `inf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID inf
    GRAMMAR    inf/core/InfGrammar.hpp
    NAMESPACE  inf::Inf
    EXTENSIONS .inf
    OUT        inf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/inf/core/*.cpp)
add_library(infCore ${_core})
target_link_libraries(infCore PUBLIC infGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/inf/cli/*.cpp)
add_executable(inf_cli ${_cli})
target_link_libraries(inf_cli
                      PRIVATE infCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/inf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/inf/lsp/main.cpp)
add_library(infLsp ${_lsp})
target_link_libraries(infLsp PUBLIC infCore cursiv::lsp)
add_executable(inf_lsp inf/lsp/main.cpp)
target_link_libraries(inf_lsp PRIVATE infLsp)

cursiv_add_benchmark(inf SOURCES inf/bench/main.cpp)

cursiv_package_language(inf)
