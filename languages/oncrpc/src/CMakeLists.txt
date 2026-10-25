# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `oncrpcGenerated` (the compiled library) and `oncrpc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID oncrpc
    GRAMMAR    oncrpc/core/OncrpcGrammar.hpp
    NAMESPACE  oncrpc::Oncrpc
    EXTENSIONS .x
    OUT        oncrpc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/core/*.cpp)
add_library(oncrpcCore ${_core})
target_link_libraries(oncrpcCore PUBLIC oncrpcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/cli/*.cpp)
add_executable(oncrpc_cli ${_cli})
target_link_libraries(oncrpc_cli
                      PRIVATE oncrpcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/lsp/main.cpp)
add_library(oncrpcLsp ${_lsp})
target_link_libraries(oncrpcLsp PUBLIC oncrpcCore cursiv::lsp)
add_executable(oncrpc_lsp oncrpc/lsp/main.cpp)
target_link_libraries(oncrpc_lsp PRIVATE oncrpcLsp)

cursiv_add_benchmark(oncrpc SOURCES oncrpc/bench/main.cpp)

cursiv_package_language(oncrpc)
