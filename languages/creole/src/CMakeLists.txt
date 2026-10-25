# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `creoleGenerated` (the compiled library) and `creole_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID creole
    GRAMMAR    creole/core/CreoleGrammar.hpp
    NAMESPACE  creole::Creole
    EXTENSIONS .creole
    OUT        creole/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/creole/core/*.cpp)
add_library(creoleCore ${_core})
target_link_libraries(creoleCore PUBLIC creoleGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/creole/cli/*.cpp)
add_executable(creole_cli ${_cli})
target_link_libraries(creole_cli
                      PRIVATE creoleCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/creole/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/creole/lsp/main.cpp)
add_library(creoleLsp ${_lsp})
target_link_libraries(creoleLsp PUBLIC creoleCore cursiv::lsp)
add_executable(creole_lsp creole/lsp/main.cpp)
target_link_libraries(creole_lsp PRIVATE creoleLsp)

cursiv_add_benchmark(creole SOURCES creole/bench/main.cpp)

cursiv_package_language(creole)
