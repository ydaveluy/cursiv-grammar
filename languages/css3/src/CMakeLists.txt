# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `css3Generated` (the compiled library) and `css3_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID css3
    GRAMMAR    css3/core/Css3Grammar.hpp
    NAMESPACE  css3::Css3
    EXTENSIONS .css
    OUT        css3/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/css3/core/*.cpp)
add_library(css3Core ${_core})
target_link_libraries(css3Core PUBLIC css3Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/css3/cli/*.cpp)
add_executable(css3_cli ${_cli})
target_link_libraries(css3_cli
                      PRIVATE css3Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/css3/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/css3/lsp/main.cpp)
add_library(css3Lsp ${_lsp})
target_link_libraries(css3Lsp PUBLIC css3Core cursiv::lsp)
add_executable(css3_lsp css3/lsp/main.cpp)
target_link_libraries(css3_lsp PRIVATE css3Lsp)

cursiv_add_benchmark(css3 SOURCES css3/bench/main.cpp)

cursiv_package_language(css3)
