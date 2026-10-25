# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `vbaGenerated` (the compiled library) and `vba_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID vba
    GRAMMAR    vba/core/VbaGrammar.hpp
    NAMESPACE  vba::Vba
    EXTENSIONS .bas .cls .frm
    OUT        vba/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vba/core/*.cpp)
add_library(vbaCore ${_core})
target_link_libraries(vbaCore PUBLIC vbaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vba/cli/*.cpp)
add_executable(vba_cli ${_cli})
target_link_libraries(vba_cli
                      PRIVATE vbaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vba/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vba/lsp/main.cpp)
add_library(vbaLsp ${_lsp})
target_link_libraries(vbaLsp PUBLIC vbaCore cursiv::lsp)
add_executable(vba_lsp vba/lsp/main.cpp)
target_link_libraries(vba_lsp PRIVATE vbaLsp)

cursiv_add_benchmark(vba SOURCES vba/bench/main.cpp)

cursiv_package_language(vba)
