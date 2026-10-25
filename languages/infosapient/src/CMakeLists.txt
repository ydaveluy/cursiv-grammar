# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `infosapientGenerated` (the compiled library) and `infosapient_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID infosapient
    GRAMMAR    infosapient/core/InfosapientGrammar.hpp
    NAMESPACE  infosapient::Infosapient
    EXTENSIONS .infosapient
    OUT        infosapient/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/core/*.cpp)
add_library(infosapientCore ${_core})
target_link_libraries(infosapientCore PUBLIC infosapientGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/cli/*.cpp)
add_executable(infosapient_cli ${_cli})
target_link_libraries(infosapient_cli
                      PRIVATE infosapientCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/lsp/main.cpp)
add_library(infosapientLsp ${_lsp})
target_link_libraries(infosapientLsp PUBLIC infosapientCore cursiv::lsp)
add_executable(infosapient_lsp infosapient/lsp/main.cpp)
target_link_libraries(infosapient_lsp PRIVATE infosapientLsp)

cursiv_add_benchmark(infosapient SOURCES infosapient/bench/main.cpp)

cursiv_package_language(infosapient)
