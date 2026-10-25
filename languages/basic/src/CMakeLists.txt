# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `basicGenerated` (the compiled library) and `basic_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID basic
    GRAMMAR    basic/core/BasicGrammar.hpp
    NAMESPACE  basic::Basic
    EXTENSIONS .bas
    OUT        basic/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/basic/core/*.cpp)
add_library(basicCore ${_core})
target_link_libraries(basicCore PUBLIC basicGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/basic/cli/*.cpp)
add_executable(basic_cli ${_cli})
target_link_libraries(basic_cli
                      PRIVATE basicCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/basic/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/basic/lsp/main.cpp)
add_library(basicLsp ${_lsp})
target_link_libraries(basicLsp PUBLIC basicCore cursiv::lsp)
add_executable(basic_lsp basic/lsp/main.cpp)
target_link_libraries(basic_lsp PRIVATE basicLsp)

cursiv_add_benchmark(basic SOURCES basic/bench/main.cpp)

cursiv_package_language(basic)
