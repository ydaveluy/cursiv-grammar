# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `miniscriptGenerated` (the compiled library) and `miniscript_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID miniscript
    GRAMMAR    miniscript/core/MiniscriptGrammar.hpp
    NAMESPACE  miniscript::Miniscript
    EXTENSIONS .ms
    OUT        miniscript/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/core/*.cpp)
add_library(miniscriptCore ${_core})
target_link_libraries(miniscriptCore PUBLIC miniscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/cli/*.cpp)
add_executable(miniscript_cli ${_cli})
target_link_libraries(miniscript_cli
                      PRIVATE miniscriptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/lsp/main.cpp)
add_library(miniscriptLsp ${_lsp})
target_link_libraries(miniscriptLsp PUBLIC miniscriptCore cursiv::lsp)
add_executable(miniscript_lsp miniscript/lsp/main.cpp)
target_link_libraries(miniscript_lsp PRIVATE miniscriptLsp)

cursiv_add_benchmark(miniscript SOURCES miniscript/bench/main.cpp)

cursiv_package_language(miniscript)
