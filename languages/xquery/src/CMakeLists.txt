# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `xqueryGenerated` (the compiled library) and `xquery_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID xquery
    GRAMMAR    xquery/core/XQueryGrammar.hpp
    NAMESPACE  xquery::XQuery
    EXTENSIONS .xq
    OUT        xquery/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xquery/core/*.cpp)
add_library(xqueryCore ${_core})
target_link_libraries(xqueryCore PUBLIC xqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xquery/cli/*.cpp)
add_executable(xquery_cli ${_cli})
target_link_libraries(xquery_cli
                      PRIVATE xqueryCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xquery/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xquery/lsp/main.cpp)
add_library(xqueryLsp ${_lsp})
target_link_libraries(xqueryLsp PUBLIC xqueryCore cursiv::lsp)
add_executable(xquery_lsp xquery/lsp/main.cpp)
target_link_libraries(xquery_lsp PRIVATE xqueryLsp)

cursiv_add_benchmark(xquery SOURCES xquery/bench/main.cpp)

cursiv_package_language(xquery)
