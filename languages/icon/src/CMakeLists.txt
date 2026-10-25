# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `iconGenerated` (the compiled library) and `icon_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID icon
    GRAMMAR    icon/core/IconGrammar.hpp
    NAMESPACE  icon::Icon
    EXTENSIONS .icn
    OUT        icon/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icon/core/*.cpp)
add_library(iconCore ${_core})
target_link_libraries(iconCore PUBLIC iconGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icon/cli/*.cpp)
add_executable(icon_cli ${_cli})
target_link_libraries(icon_cli
                      PRIVATE iconCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icon/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/icon/lsp/main.cpp)
add_library(iconLsp ${_lsp})
target_link_libraries(iconLsp PUBLIC iconCore cursiv::lsp)
add_executable(icon_lsp icon/lsp/main.cpp)
target_link_libraries(icon_lsp PRIVATE iconLsp)

cursiv_add_benchmark(icon SOURCES icon/bench/main.cpp)

cursiv_package_language(icon)
