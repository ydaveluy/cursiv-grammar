# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `abbGenerated` (the compiled library) and `abb_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID abb
    GRAMMAR    abb/core/AbbGrammar.hpp
    NAMESPACE  abb::Abb
    EXTENSIONS .sys
    OUT        abb/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abb/core/*.cpp)
add_library(abbCore ${_core})
target_link_libraries(abbCore PUBLIC abbGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abb/cli/*.cpp)
add_executable(abb_cli ${_cli})
target_link_libraries(abb_cli
                      PRIVATE abbCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abb/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/abb/lsp/main.cpp)
add_library(abbLsp ${_lsp})
target_link_libraries(abbLsp PUBLIC abbCore cursiv::lsp)
add_executable(abb_lsp abb/lsp/main.cpp)
target_link_libraries(abb_lsp PRIVATE abbLsp)

cursiv_add_benchmark(abb SOURCES abb/bench/main.cpp)

cursiv_package_language(abb)
