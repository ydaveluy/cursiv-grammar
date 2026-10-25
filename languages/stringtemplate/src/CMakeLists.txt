# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `stringtemplateGenerated` (the compiled library) and `stringtemplate_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID stringtemplate
    GRAMMAR    stringtemplate/core/StringtemplateGrammar.hpp
    NAMESPACE  stringtemplate::Stringtemplate
    EXTENSIONS .st
    OUT        stringtemplate/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/core/*.cpp)
add_library(stringtemplateCore ${_core})
target_link_libraries(stringtemplateCore PUBLIC stringtemplateGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/cli/*.cpp)
add_executable(stringtemplate_cli ${_cli})
target_link_libraries(stringtemplate_cli
                      PRIVATE stringtemplateCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/lsp/main.cpp)
add_library(stringtemplateLsp ${_lsp})
target_link_libraries(stringtemplateLsp PUBLIC stringtemplateCore cursiv::lsp)
add_executable(stringtemplate_lsp stringtemplate/lsp/main.cpp)
target_link_libraries(stringtemplate_lsp PRIVATE stringtemplateLsp)

cursiv_add_benchmark(stringtemplate SOURCES stringtemplate/bench/main.cpp)

cursiv_package_language(stringtemplate)
