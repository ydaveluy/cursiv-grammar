# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tealGenerated` (the compiled library) and `teal_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID teal
    GRAMMAR    teal/core/TealGrammar.hpp
    NAMESPACE  teal::Teal
    EXTENSIONS .tl
    OUT        teal/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/teal/core/*.cpp)
add_library(tealCore ${_core})
target_link_libraries(tealCore PUBLIC tealGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/teal/cli/*.cpp)
add_executable(teal_cli ${_cli})
target_link_libraries(teal_cli
                      PRIVATE tealCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/teal/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/teal/lsp/main.cpp)
add_library(tealLsp ${_lsp})
target_link_libraries(tealLsp PUBLIC tealCore cursiv::lsp)
add_executable(teal_lsp teal/lsp/main.cpp)
target_link_libraries(teal_lsp PRIVATE tealLsp)

cursiv_add_benchmark(teal SOURCES teal/bench/main.cpp)

cursiv_package_language(teal)
