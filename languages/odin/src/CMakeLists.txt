# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `odinGenerated` (the compiled library) and `odin_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID odin
    GRAMMAR    odin/core/OdinGrammar.hpp
    NAMESPACE  odin::Odin
    EXTENSIONS .odin
    OUT        odin/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/odin/core/*.cpp)
add_library(odinCore ${_core})
target_link_libraries(odinCore PUBLIC odinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/odin/cli/*.cpp)
add_executable(odin_cli ${_cli})
target_link_libraries(odin_cli
                      PRIVATE odinCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/odin/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/odin/lsp/main.cpp)
add_library(odinLsp ${_lsp})
target_link_libraries(odinLsp PUBLIC odinCore cursiv::lsp)
add_executable(odin_lsp odin/lsp/main.cpp)
target_link_libraries(odin_lsp PRIVATE odinLsp)

cursiv_add_benchmark(odin SOURCES odin/bench/main.cpp)

cursiv_package_language(odin)
