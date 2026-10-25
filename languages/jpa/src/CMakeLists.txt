# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `jpaGenerated` (the compiled library) and `jpa_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID jpa
    GRAMMAR    jpa/core/JpaGrammar.hpp
    NAMESPACE  jpa::Jpa
    EXTENSIONS .jpql
    OUT        jpa/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jpa/core/*.cpp)
add_library(jpaCore ${_core})
target_link_libraries(jpaCore PUBLIC jpaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jpa/cli/*.cpp)
add_executable(jpa_cli ${_cli})
target_link_libraries(jpa_cli
                      PRIVATE jpaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jpa/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/jpa/lsp/main.cpp)
add_library(jpaLsp ${_lsp})
target_link_libraries(jpaLsp PUBLIC jpaCore cursiv::lsp)
add_executable(jpa_lsp jpa/lsp/main.cpp)
target_link_libraries(jpa_lsp PRIVATE jpaLsp)

cursiv_add_benchmark(jpa SOURCES jpa/bench/main.cpp)

cursiv_package_language(jpa)
