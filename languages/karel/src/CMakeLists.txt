# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `karelGenerated` (the compiled library) and `karel_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID karel
    GRAMMAR    karel/core/KarelGrammar.hpp
    NAMESPACE  karel::Karel
    EXTENSIONS .karel
    OUT        karel/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/karel/core/*.cpp)
add_library(karelCore ${_core})
target_link_libraries(karelCore PUBLIC karelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/karel/cli/*.cpp)
add_executable(karel_cli ${_cli})
target_link_libraries(karel_cli
                      PRIVATE karelCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/karel/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/karel/lsp/main.cpp)
add_library(karelLsp ${_lsp})
target_link_libraries(karelLsp PUBLIC karelCore cursiv::lsp)
add_executable(karel_lsp karel/lsp/main.cpp)
target_link_libraries(karel_lsp PRIVATE karelLsp)

cursiv_add_benchmark(karel SOURCES karel/bench/main.cpp)

cursiv_package_language(karel)
