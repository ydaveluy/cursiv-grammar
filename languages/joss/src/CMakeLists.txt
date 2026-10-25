# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `jossGenerated` (the compiled library) and `joss_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID joss
    GRAMMAR    joss/core/JossGrammar.hpp
    NAMESPACE  joss::Joss
    EXTENSIONS .joss
    OUT        joss/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/joss/core/*.cpp)
add_library(jossCore ${_core})
target_link_libraries(jossCore PUBLIC jossGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/joss/cli/*.cpp)
add_executable(joss_cli ${_cli})
target_link_libraries(joss_cli
                      PRIVATE jossCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/joss/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/joss/lsp/main.cpp)
add_library(jossLsp ${_lsp})
target_link_libraries(jossLsp PUBLIC jossCore cursiv::lsp)
add_executable(joss_lsp joss/lsp/main.cpp)
target_link_libraries(joss_lsp PRIVATE jossLsp)

cursiv_add_benchmark(joss SOURCES joss/bench/main.cpp)

cursiv_package_language(joss)
