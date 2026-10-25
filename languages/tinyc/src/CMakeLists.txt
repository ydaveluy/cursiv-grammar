# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tinycGenerated` (the compiled library) and `tinyc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tinyc
    GRAMMAR    tinyc/core/TinycGrammar.hpp
    NAMESPACE  tinyc::Tinyc
    EXTENSIONS .tinyc
    OUT        tinyc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/core/*.cpp)
add_library(tinycCore ${_core})
target_link_libraries(tinycCore PUBLIC tinycGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/cli/*.cpp)
add_executable(tinyc_cli ${_cli})
target_link_libraries(tinyc_cli
                      PRIVATE tinycCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/lsp/main.cpp)
add_library(tinycLsp ${_lsp})
target_link_libraries(tinycLsp PUBLIC tinycCore cursiv::lsp)
add_executable(tinyc_lsp tinyc/lsp/main.cpp)
target_link_libraries(tinyc_lsp PRIVATE tinycLsp)

cursiv_add_benchmark(tinyc SOURCES tinyc/bench/main.cpp)

cursiv_package_language(tinyc)
