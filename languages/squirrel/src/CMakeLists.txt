# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `squirrelGenerated` (the compiled library) and `squirrel_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID squirrel
    GRAMMAR    squirrel/core/SquirrelGrammar.hpp
    NAMESPACE  squirrel::Squirrel
    EXTENSIONS .nut
    OUT        squirrel/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/core/*.cpp)
add_library(squirrelCore ${_core})
target_link_libraries(squirrelCore PUBLIC squirrelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/cli/*.cpp)
add_executable(squirrel_cli ${_cli})
target_link_libraries(squirrel_cli
                      PRIVATE squirrelCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/lsp/main.cpp)
add_library(squirrelLsp ${_lsp})
target_link_libraries(squirrelLsp PUBLIC squirrelCore cursiv::lsp)
add_executable(squirrel_lsp squirrel/lsp/main.cpp)
target_link_libraries(squirrel_lsp PRIVATE squirrelLsp)

cursiv_add_benchmark(squirrel SOURCES squirrel/bench/main.cpp)

cursiv_package_language(squirrel)
