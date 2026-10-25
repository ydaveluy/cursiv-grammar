# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `kqueryGenerated` (the compiled library) and `kquery_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID kquery
    GRAMMAR    kquery/core/KqueryGrammar.hpp
    NAMESPACE  kquery::Kquery
    EXTENSIONS .kquery
    OUT        kquery/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kquery/core/*.cpp)
add_library(kqueryCore ${_core})
target_link_libraries(kqueryCore PUBLIC kqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kquery/cli/*.cpp)
add_executable(kquery_cli ${_cli})
target_link_libraries(kquery_cli
                      PRIVATE kqueryCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kquery/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kquery/lsp/main.cpp)
add_library(kqueryLsp ${_lsp})
target_link_libraries(kqueryLsp PUBLIC kqueryCore cursiv::lsp)
add_executable(kquery_lsp kquery/lsp/main.cpp)
target_link_libraries(kquery_lsp PRIVATE kqueryLsp)

cursiv_add_benchmark(kquery SOURCES kquery/bench/main.cpp)

cursiv_package_language(kquery)
