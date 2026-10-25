# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `powerbuilderdwGenerated` (the compiled library) and `powerbuilderdw_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID powerbuilderdw
    GRAMMAR    powerbuilderdw/core/PowerbuilderdwGrammar.hpp
    NAMESPACE  powerbuilderdw::Powerbuilderdw
    EXTENSIONS .srd
    OUT        powerbuilderdw/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/core/*.cpp)
add_library(powerbuilderdwCore ${_core})
target_link_libraries(powerbuilderdwCore PUBLIC powerbuilderdwGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/cli/*.cpp)
add_executable(powerbuilderdw_cli ${_cli})
target_link_libraries(powerbuilderdw_cli
                      PRIVATE powerbuilderdwCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/lsp/main.cpp)
add_library(powerbuilderdwLsp ${_lsp})
target_link_libraries(powerbuilderdwLsp PUBLIC powerbuilderdwCore cursiv::lsp)
add_executable(powerbuilderdw_lsp powerbuilderdw/lsp/main.cpp)
target_link_libraries(powerbuilderdw_lsp PRIVATE powerbuilderdwLsp)

cursiv_add_benchmark(powerbuilderdw SOURCES powerbuilderdw/bench/main.cpp)

cursiv_package_language(powerbuilderdw)
