# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `stlGenerated` (the compiled library) and `stl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID stl
    GRAMMAR    stl/core/StlGrammar.hpp
    NAMESPACE  stl::Stl
    EXTENSIONS .stl
    OUT        stl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stl/core/*.cpp)
add_library(stlCore ${_core})
target_link_libraries(stlCore PUBLIC stlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stl/cli/*.cpp)
add_executable(stl_cli ${_cli})
target_link_libraries(stl_cli
                      PRIVATE stlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stl/lsp/main.cpp)
add_library(stlLsp ${_lsp})
target_link_libraries(stlLsp PUBLIC stlCore cursiv::lsp)
add_executable(stl_lsp stl/lsp/main.cpp)
target_link_libraries(stl_lsp PRIVATE stlLsp)

cursiv_add_benchmark(stl SOURCES stl/bench/main.cpp)

cursiv_package_language(stl)
