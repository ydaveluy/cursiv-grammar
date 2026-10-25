# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gdscriptGenerated` (the compiled library) and `gdscript_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gdscript
    GRAMMAR    gdscript/core/GdscriptGrammar.hpp
    NAMESPACE  gdscript::Gdscript
    EXTENSIONS .gd
    OUT        gdscript/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/core/*.cpp)
add_library(gdscriptCore ${_core})
target_link_libraries(gdscriptCore PUBLIC gdscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/cli/*.cpp)
add_executable(gdscript_cli ${_cli})
target_link_libraries(gdscript_cli
                      PRIVATE gdscriptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/lsp/main.cpp)
add_library(gdscriptLsp ${_lsp})
target_link_libraries(gdscriptLsp PUBLIC gdscriptCore cursiv::lsp)
add_executable(gdscript_lsp gdscript/lsp/main.cpp)
target_link_libraries(gdscript_lsp PRIVATE gdscriptLsp)

cursiv_add_benchmark(gdscript SOURCES gdscript/bench/main.cpp)

cursiv_package_language(gdscript)
