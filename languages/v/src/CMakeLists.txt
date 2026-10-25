# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `vGenerated` (the compiled library) and `v_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID v
    GRAMMAR    v/core/VGrammar.hpp
    NAMESPACE  v::V
    EXTENSIONS .v
    OUT        v/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/v/core/*.cpp)
add_library(vCore ${_core})
target_link_libraries(vCore PUBLIC vGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/v/cli/*.cpp)
add_executable(v_cli ${_cli})
target_link_libraries(v_cli
                      PRIVATE vCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/v/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/v/lsp/main.cpp)
add_library(vLsp ${_lsp})
target_link_libraries(vLsp PUBLIC vCore cursiv::lsp)
add_executable(v_lsp v/lsp/main.cpp)
target_link_libraries(v_lsp PRIVATE vLsp)

cursiv_add_benchmark(v SOURCES v/bench/main.cpp)

cursiv_package_language(v)
