# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tnsnamesGenerated` (the compiled library) and `tnsnames_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tnsnames
    GRAMMAR    tnsnames/core/TnsnamesGrammar.hpp
    NAMESPACE  tnsnames::Tnsnames
    EXTENSIONS .ora
    OUT        tnsnames/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/core/*.cpp)
add_library(tnsnamesCore ${_core})
target_link_libraries(tnsnamesCore PUBLIC tnsnamesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/cli/*.cpp)
add_executable(tnsnames_cli ${_cli})
target_link_libraries(tnsnames_cli
                      PRIVATE tnsnamesCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/lsp/main.cpp)
add_library(tnsnamesLsp ${_lsp})
target_link_libraries(tnsnamesLsp PUBLIC tnsnamesCore cursiv::lsp)
add_executable(tnsnames_lsp tnsnames/lsp/main.cpp)
target_link_libraries(tnsnames_lsp PRIVATE tnsnamesLsp)

cursiv_add_benchmark(tnsnames SOURCES tnsnames/bench/main.cpp)

cursiv_package_language(tnsnames)
