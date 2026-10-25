# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ctoGenerated` (the compiled library) and `cto_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cto
    GRAMMAR    cto/core/CtoGrammar.hpp
    NAMESPACE  cto::Cto
    EXTENSIONS .cto
    OUT        cto/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cto/core/*.cpp)
add_library(ctoCore ${_core})
target_link_libraries(ctoCore PUBLIC ctoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cto/cli/*.cpp)
add_executable(cto_cli ${_cli})
target_link_libraries(cto_cli
                      PRIVATE ctoCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cto/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cto/lsp/main.cpp)
add_library(ctoLsp ${_lsp})
target_link_libraries(ctoLsp PUBLIC ctoCore cursiv::lsp)
add_executable(cto_lsp cto/lsp/main.cpp)
target_link_libraries(cto_lsp PRIVATE ctoLsp)

cursiv_add_benchmark(cto SOURCES cto/bench/main.cpp)

cursiv_package_language(cto)
