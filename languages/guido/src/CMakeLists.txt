# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `guidoGenerated` (the compiled library) and `guido_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID guido
    GRAMMAR    guido/core/GuidoGrammar.hpp
    NAMESPACE  guido::Guido
    EXTENSIONS .gmn
    OUT        guido/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guido/core/*.cpp)
add_library(guidoCore ${_core})
target_link_libraries(guidoCore PUBLIC guidoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guido/cli/*.cpp)
add_executable(guido_cli ${_cli})
target_link_libraries(guido_cli
                      PRIVATE guidoCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guido/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/guido/lsp/main.cpp)
add_library(guidoLsp ${_lsp})
target_link_libraries(guidoLsp PUBLIC guidoCore cursiv::lsp)
add_executable(guido_lsp guido/lsp/main.cpp)
target_link_libraries(guido_lsp PRIVATE guidoLsp)

cursiv_add_benchmark(guido SOURCES guido/bench/main.cpp)

cursiv_package_language(guido)
