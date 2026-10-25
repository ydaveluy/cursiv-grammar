# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `angelscriptGenerated` (the compiled library) and `angelscript_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID angelscript
    GRAMMAR    angelscript/core/AngelscriptGrammar.hpp
    NAMESPACE  angelscript::Angelscript
    EXTENSIONS .as
    OUT        angelscript/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/core/*.cpp)
add_library(angelscriptCore ${_core})
target_link_libraries(angelscriptCore PUBLIC angelscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/cli/*.cpp)
add_executable(angelscript_cli ${_cli})
target_link_libraries(angelscript_cli
                      PRIVATE angelscriptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/lsp/main.cpp)
add_library(angelscriptLsp ${_lsp})
target_link_libraries(angelscriptLsp PUBLIC angelscriptCore cursiv::lsp)
add_executable(angelscript_lsp angelscript/lsp/main.cpp)
target_link_libraries(angelscript_lsp PRIVATE angelscriptLsp)

cursiv_add_benchmark(angelscript SOURCES angelscript/bench/main.cpp)

cursiv_package_language(angelscript)
