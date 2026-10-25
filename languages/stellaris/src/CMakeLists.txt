# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `stellarisGenerated` (the compiled library) and `stellaris_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID stellaris
    GRAMMAR    stellaris/core/StellarisGrammar.hpp
    NAMESPACE  stellaris::Stellaris
    EXTENSIONS .stellaris
    OUT        stellaris/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/core/*.cpp)
add_library(stellarisCore ${_core})
target_link_libraries(stellarisCore PUBLIC stellarisGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/cli/*.cpp)
add_executable(stellaris_cli ${_cli})
target_link_libraries(stellaris_cli
                      PRIVATE stellarisCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/lsp/main.cpp)
add_library(stellarisLsp ${_lsp})
target_link_libraries(stellarisLsp PUBLIC stellarisCore cursiv::lsp)
add_executable(stellaris_lsp stellaris/lsp/main.cpp)
target_link_libraries(stellaris_lsp PRIVATE stellarisLsp)

cursiv_add_benchmark(stellaris SOURCES stellaris/bench/main.cpp)

cursiv_package_language(stellaris)
