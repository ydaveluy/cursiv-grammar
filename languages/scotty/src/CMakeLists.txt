# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `scottyGenerated` (the compiled library) and `scotty_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID scotty
    GRAMMAR    scotty/core/ScottyGrammar.hpp
    NAMESPACE  scotty::Scotty
    EXTENSIONS .scotty
    OUT        scotty/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scotty/core/*.cpp)
add_library(scottyCore ${_core})
target_link_libraries(scottyCore PUBLIC scottyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scotty/cli/*.cpp)
add_executable(scotty_cli ${_cli})
target_link_libraries(scotty_cli
                      PRIVATE scottyCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scotty/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scotty/lsp/main.cpp)
add_library(scottyLsp ${_lsp})
target_link_libraries(scottyLsp PUBLIC scottyCore cursiv::lsp)
add_executable(scotty_lsp scotty/lsp/main.cpp)
target_link_libraries(scotty_lsp PRIVATE scottyLsp)

cursiv_add_benchmark(scotty SOURCES scotty/bench/main.cpp)

cursiv_package_language(scotty)
