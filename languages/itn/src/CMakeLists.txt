# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `itnGenerated` (the compiled library) and `itn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID itn
    GRAMMAR    itn/core/ItnGrammar.hpp
    NAMESPACE  itn::Itn
    EXTENSIONS .itn
    OUT        itn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/itn/core/*.cpp)
add_library(itnCore ${_core})
target_link_libraries(itnCore PUBLIC itnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/itn/cli/*.cpp)
add_executable(itn_cli ${_cli})
target_link_libraries(itn_cli
                      PRIVATE itnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/itn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/itn/lsp/main.cpp)
add_library(itnLsp ${_lsp})
target_link_libraries(itnLsp PUBLIC itnCore cursiv::lsp)
add_executable(itn_lsp itn/lsp/main.cpp)
target_link_libraries(itn_lsp PRIVATE itnLsp)

cursiv_add_benchmark(itn SOURCES itn/bench/main.cpp)

cursiv_package_language(itn)
