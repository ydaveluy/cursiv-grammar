# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `powerqueryGenerated` (the compiled library) and `powerquery_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID powerquery
    GRAMMAR    powerquery/core/PowerqueryGrammar.hpp
    NAMESPACE  powerquery::Powerquery
    EXTENSIONS .pq
    OUT        powerquery/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/core/*.cpp)
add_library(powerqueryCore ${_core})
target_link_libraries(powerqueryCore PUBLIC powerqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/cli/*.cpp)
add_executable(powerquery_cli ${_cli})
target_link_libraries(powerquery_cli
                      PRIVATE powerqueryCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/lsp/main.cpp)
add_library(powerqueryLsp ${_lsp})
target_link_libraries(powerqueryLsp PUBLIC powerqueryCore cursiv::lsp)
add_executable(powerquery_lsp powerquery/lsp/main.cpp)
target_link_libraries(powerquery_lsp PRIVATE powerqueryLsp)

cursiv_add_benchmark(powerquery SOURCES powerquery/bench/main.cpp)

cursiv_package_language(powerquery)
