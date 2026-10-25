# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `postalcodeGenerated` (the compiled library) and `postalcode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID postalcode
    GRAMMAR    postalcode/core/PostalcodeGrammar.hpp
    NAMESPACE  postalcode::Postalcode
    EXTENSIONS .postalcode
    OUT        postalcode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/core/*.cpp)
add_library(postalcodeCore ${_core})
target_link_libraries(postalcodeCore PUBLIC postalcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/cli/*.cpp)
add_executable(postalcode_cli ${_cli})
target_link_libraries(postalcode_cli
                      PRIVATE postalcodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/lsp/main.cpp)
add_library(postalcodeLsp ${_lsp})
target_link_libraries(postalcodeLsp PUBLIC postalcodeCore cursiv::lsp)
add_executable(postalcode_lsp postalcode/lsp/main.cpp)
target_link_libraries(postalcode_lsp PRIVATE postalcodeLsp)

cursiv_add_benchmark(postalcode SOURCES postalcode/bench/main.cpp)

cursiv_package_language(postalcode)
