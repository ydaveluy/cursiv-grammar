# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `datalogGenerated` (the compiled library) and `datalog_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID datalog
    GRAMMAR    datalog/core/DatalogGrammar.hpp
    NAMESPACE  datalog::Datalog
    EXTENSIONS .dl
    OUT        datalog/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/datalog/core/*.cpp)
add_library(datalogCore ${_core})
target_link_libraries(datalogCore PUBLIC datalogGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/datalog/cli/*.cpp)
add_executable(datalog_cli ${_cli})
target_link_libraries(datalog_cli
                      PRIVATE datalogCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/datalog/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/datalog/lsp/main.cpp)
add_library(datalogLsp ${_lsp})
target_link_libraries(datalogLsp PUBLIC datalogCore cursiv::lsp)
add_executable(datalog_lsp datalog/lsp/main.cpp)
target_link_libraries(datalog_lsp PRIVATE datalogLsp)

cursiv_add_benchmark(datalog SOURCES datalog/bench/main.cpp)

cursiv_package_language(datalog)
