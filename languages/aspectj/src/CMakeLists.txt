# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `aspectjGenerated` (the compiled library) and `aspectj_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID aspectj
    GRAMMAR    aspectj/core/AspectjGrammar.hpp
    NAMESPACE  aspectj::Aspectj
    EXTENSIONS .aj
    OUT        aspectj/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/core/*.cpp)
add_library(aspectjCore ${_core})
target_link_libraries(aspectjCore PUBLIC aspectjGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/cli/*.cpp)
add_executable(aspectj_cli ${_cli})
target_link_libraries(aspectj_cli
                      PRIVATE aspectjCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/lsp/main.cpp)
add_library(aspectjLsp ${_lsp})
target_link_libraries(aspectjLsp PUBLIC aspectjCore cursiv::lsp)
add_executable(aspectj_lsp aspectj/lsp/main.cpp)
target_link_libraries(aspectj_lsp PRIVATE aspectjLsp)

cursiv_add_benchmark(aspectj SOURCES aspectj/bench/main.cpp)

cursiv_package_language(aspectj)
