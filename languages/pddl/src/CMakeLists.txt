# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pddlGenerated` (the compiled library) and `pddl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pddl
    GRAMMAR    pddl/core/PddlGrammar.hpp
    NAMESPACE  pddl::Pddl
    EXTENSIONS .pddl
    OUT        pddl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pddl/core/*.cpp)
add_library(pddlCore ${_core})
target_link_libraries(pddlCore PUBLIC pddlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pddl/cli/*.cpp)
add_executable(pddl_cli ${_cli})
target_link_libraries(pddl_cli
                      PRIVATE pddlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pddl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pddl/lsp/main.cpp)
add_library(pddlLsp ${_lsp})
target_link_libraries(pddlLsp PUBLIC pddlCore cursiv::lsp)
add_executable(pddl_lsp pddl/lsp/main.cpp)
target_link_libraries(pddl_lsp PRIVATE pddlLsp)

cursiv_add_benchmark(pddl SOURCES pddl/bench/main.cpp)

cursiv_package_language(pddl)
