# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `unrealangelscriptGenerated` (the compiled library) and `unrealangelscript_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID unrealangelscript
    GRAMMAR    unrealangelscript/core/UnrealAngelscriptGrammar.hpp
    NAMESPACE  unrealangelscript::UnrealAngelscript
    EXTENSIONS .as
    OUT        unrealangelscript/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/core/*.cpp)
add_library(unrealangelscriptCore ${_core})
target_link_libraries(unrealangelscriptCore PUBLIC unrealangelscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/cli/*.cpp)
add_executable(unrealangelscript_cli ${_cli})
target_link_libraries(unrealangelscript_cli
                      PRIVATE unrealangelscriptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/lsp/main.cpp)
add_library(unrealangelscriptLsp ${_lsp})
target_link_libraries(unrealangelscriptLsp PUBLIC unrealangelscriptCore cursiv::lsp)
add_executable(unrealangelscript_lsp unrealangelscript/lsp/main.cpp)
target_link_libraries(unrealangelscript_lsp PRIVATE unrealangelscriptLsp)

cursiv_add_benchmark(unrealangelscript SOURCES unrealangelscript/bench/main.cpp)

cursiv_package_language(unrealangelscript)
