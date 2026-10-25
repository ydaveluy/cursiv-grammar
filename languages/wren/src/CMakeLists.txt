# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `wrenGenerated` (the compiled library) and `wren_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wren
    GRAMMAR    wren/core/WrenGrammar.hpp
    NAMESPACE  wren::Wren
    EXTENSIONS .wren
    OUT        wren/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wren/core/*.cpp)
add_library(wrenCore ${_core})
target_link_libraries(wrenCore PUBLIC wrenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wren/cli/*.cpp)
add_executable(wren_cli ${_cli})
target_link_libraries(wren_cli
                      PRIVATE wrenCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wren/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wren/lsp/main.cpp)
add_library(wrenLsp ${_lsp})
target_link_libraries(wrenLsp PUBLIC wrenCore cursiv::lsp)
add_executable(wren_lsp wren/lsp/main.cpp)
target_link_libraries(wren_lsp PRIVATE wrenLsp)

cursiv_add_benchmark(wren SOURCES wren/bench/main.cpp)

cursiv_package_language(wren)
