# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bGenerated` (the compiled library) and `b_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID b
    GRAMMAR    b/core/BGrammar.hpp
    NAMESPACE  b::B
    EXTENSIONS .b
    OUT        b/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/b/core/*.cpp)
add_library(bCore ${_core})
target_link_libraries(bCore PUBLIC bGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/b/cli/*.cpp)
add_executable(b_cli ${_cli})
target_link_libraries(b_cli
                      PRIVATE bCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/b/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/b/lsp/main.cpp)
add_library(bLsp ${_lsp})
target_link_libraries(bLsp PUBLIC bCore cursiv::lsp)
add_executable(b_lsp b/lsp/main.cpp)
target_link_libraries(b_lsp PRIVATE bLsp)

cursiv_add_benchmark(b SOURCES b/bench/main.cpp)

cursiv_package_language(b)
