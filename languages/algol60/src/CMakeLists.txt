# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `algol60Generated` (the compiled library) and `algol60_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID algol60
    GRAMMAR    algol60/core/Algol60Grammar.hpp
    NAMESPACE  algol60::Algol60
    EXTENSIONS .a60 .alg
    OUT        algol60/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/algol60/core/*.cpp)
add_library(algol60Core ${_core})
target_link_libraries(algol60Core PUBLIC algol60Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/algol60/cli/*.cpp)
add_executable(algol60_cli ${_cli})
target_link_libraries(algol60_cli
                      PRIVATE algol60Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/algol60/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/algol60/lsp/main.cpp)
add_library(algol60Lsp ${_lsp})
target_link_libraries(algol60Lsp PUBLIC algol60Core cursiv::lsp)
add_executable(algol60_lsp algol60/lsp/main.cpp)
target_link_libraries(algol60_lsp PRIVATE algol60Lsp)

cursiv_add_benchmark(algol60 SOURCES algol60/bench/main.cpp)

cursiv_package_language(algol60)
