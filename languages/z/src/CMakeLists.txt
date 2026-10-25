# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `zGenerated` (the compiled library) and `z_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID z
    GRAMMAR    z/core/ZGrammar.hpp
    NAMESPACE  z::Z
    EXTENSIONS .zed
    OUT        z/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/z/core/*.cpp)
add_library(zCore ${_core})
target_link_libraries(zCore PUBLIC zGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/z/cli/*.cpp)
add_executable(z_cli ${_cli})
target_link_libraries(z_cli
                      PRIVATE zCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/z/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/z/lsp/main.cpp)
add_library(zLsp ${_lsp})
target_link_libraries(zLsp PUBLIC zCore cursiv::lsp)
add_executable(z_lsp z/lsp/main.cpp)
target_link_libraries(z_lsp PRIVATE zLsp)

cursiv_add_benchmark(z SOURCES z/bench/main.cpp)

cursiv_package_language(z)
