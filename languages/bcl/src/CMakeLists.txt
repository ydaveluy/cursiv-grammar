# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bclGenerated` (the compiled library) and `bcl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bcl
    GRAMMAR    bcl/core/BclGrammar.hpp
    NAMESPACE  bcl::Bcl
    EXTENSIONS .bcl
    OUT        bcl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcl/core/*.cpp)
add_library(bclCore ${_core})
target_link_libraries(bclCore PUBLIC bclGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcl/cli/*.cpp)
add_executable(bcl_cli ${_cli})
target_link_libraries(bcl_cli
                      PRIVATE bclCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bcl/lsp/main.cpp)
add_library(bclLsp ${_lsp})
target_link_libraries(bclLsp PUBLIC bclCore cursiv::lsp)
add_executable(bcl_lsp bcl/lsp/main.cpp)
target_link_libraries(bcl_lsp PRIVATE bclLsp)

cursiv_add_benchmark(bcl SOURCES bcl/bench/main.cpp)

cursiv_package_language(bcl)
