# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `atlGenerated` (the compiled library) and `atl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID atl
    GRAMMAR    atl/core/AtlGrammar.hpp
    NAMESPACE  atl::Atl
    EXTENSIONS .atl
    OUT        atl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/atl/core/*.cpp)
add_library(atlCore ${_core})
target_link_libraries(atlCore PUBLIC atlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/atl/cli/*.cpp)
add_executable(atl_cli ${_cli})
target_link_libraries(atl_cli
                      PRIVATE atlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/atl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/atl/lsp/main.cpp)
add_library(atlLsp ${_lsp})
target_link_libraries(atlLsp PUBLIC atlCore cursiv::lsp)
add_executable(atl_lsp atl/lsp/main.cpp)
target_link_libraries(atl_lsp PRIVATE atlLsp)

cursiv_add_benchmark(atl SOURCES atl/bench/main.cpp)

cursiv_package_language(atl)
