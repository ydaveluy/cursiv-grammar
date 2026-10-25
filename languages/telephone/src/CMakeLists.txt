# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `telephoneGenerated` (the compiled library) and `telephone_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID telephone
    GRAMMAR    telephone/core/TelephoneGrammar.hpp
    NAMESPACE  telephone::Telephone
    EXTENSIONS .tel
    OUT        telephone/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/telephone/core/*.cpp)
add_library(telephoneCore ${_core})
target_link_libraries(telephoneCore PUBLIC telephoneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/telephone/cli/*.cpp)
add_executable(telephone_cli ${_cli})
target_link_libraries(telephone_cli
                      PRIVATE telephoneCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/telephone/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/telephone/lsp/main.cpp)
add_library(telephoneLsp ${_lsp})
target_link_libraries(telephoneLsp PUBLIC telephoneCore cursiv::lsp)
add_executable(telephone_lsp telephone/lsp/main.cpp)
target_link_libraries(telephone_lsp PRIVATE telephoneLsp)

cursiv_add_benchmark(telephone SOURCES telephone/bench/main.cpp)

cursiv_package_language(telephone)
