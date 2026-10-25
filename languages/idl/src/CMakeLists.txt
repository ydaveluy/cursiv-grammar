# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `idlGenerated` (the compiled library) and `idl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID idl
    GRAMMAR    idl/core/IdlGrammar.hpp
    NAMESPACE  idl::Idl
    EXTENSIONS .idl
    OUT        idl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/idl/core/*.cpp)
add_library(idlCore ${_core})
target_link_libraries(idlCore PUBLIC idlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/idl/cli/*.cpp)
add_executable(idl_cli ${_cli})
target_link_libraries(idl_cli
                      PRIVATE idlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/idl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/idl/lsp/main.cpp)
add_library(idlLsp ${_lsp})
target_link_libraries(idlLsp PUBLIC idlCore cursiv::lsp)
add_executable(idl_lsp idl/lsp/main.cpp)
target_link_libraries(idl_lsp PRIVATE idlLsp)

cursiv_add_benchmark(idl SOURCES idl/bench/main.cpp)

cursiv_package_language(idl)
