# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `provnGenerated` (the compiled library) and `provn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID provn
    GRAMMAR    provn/core/ProvNGrammar.hpp
    NAMESPACE  provn::ProvN
    EXTENSIONS .provn
    OUT        provn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/provn/core/*.cpp)
add_library(provnCore ${_core})
target_link_libraries(provnCore PUBLIC provnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/provn/cli/*.cpp)
add_executable(provn_cli ${_cli})
target_link_libraries(provn_cli
                      PRIVATE provnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/provn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/provn/lsp/main.cpp)
add_library(provnLsp ${_lsp})
target_link_libraries(provnLsp PUBLIC provnCore cursiv::lsp)
add_executable(provn_lsp provn/lsp/main.cpp)
target_link_libraries(provn_lsp PRIVATE provnLsp)

cursiv_add_benchmark(provn SOURCES provn/bench/main.cpp)

cursiv_package_language(provn)
