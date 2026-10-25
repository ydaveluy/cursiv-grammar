# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `langiumGenerated` (the compiled library) and `langium_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID langium
    GRAMMAR    langium/core/LangiumGrammar.hpp
    NAMESPACE  langium::Langium
    EXTENSIONS .langium
    OUT        langium/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/langium/core/*.cpp)
add_library(langiumCore ${_core})
target_link_libraries(langiumCore PUBLIC langiumGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/langium/cli/*.cpp)
add_executable(langium_cli ${_cli})
target_link_libraries(langium_cli
                      PRIVATE langiumCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/langium/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/langium/lsp/main.cpp)
add_library(langiumLsp ${_lsp})
target_link_libraries(langiumLsp PUBLIC langiumCore cursiv::lsp)
add_executable(langium_lsp langium/lsp/main.cpp)
target_link_libraries(langium_lsp PRIVATE langiumLsp)

cursiv_add_benchmark(langium SOURCES langium/bench/main.cpp)

cursiv_package_language(langium)
