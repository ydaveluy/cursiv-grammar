# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lpcGenerated` (the compiled library) and `lpc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lpc
    GRAMMAR    lpc/core/LpcGrammar.hpp
    NAMESPACE  lpc::Lpc
    EXTENSIONS .lpc
    OUT        lpc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lpc/core/*.cpp)
add_library(lpcCore ${_core})
target_link_libraries(lpcCore PUBLIC lpcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lpc/cli/*.cpp)
add_executable(lpc_cli ${_cli})
target_link_libraries(lpc_cli
                      PRIVATE lpcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lpc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lpc/lsp/main.cpp)
add_library(lpcLsp ${_lsp})
target_link_libraries(lpcLsp PUBLIC lpcCore cursiv::lsp)
add_executable(lpc_lsp lpc/lsp/main.cpp)
target_link_libraries(lpc_lsp PRIVATE lpcLsp)

cursiv_add_benchmark(lpc SOURCES lpc/bench/main.cpp)

cursiv_package_language(lpc)
