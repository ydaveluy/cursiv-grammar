# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gmlGenerated` (the compiled library) and `gml_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gml
    GRAMMAR    gml/core/GmlGrammar.hpp
    NAMESPACE  gml::Gml
    EXTENSIONS .gml
    OUT        gml/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gml/core/*.cpp)
add_library(gmlCore ${_core})
target_link_libraries(gmlCore PUBLIC gmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gml/cli/*.cpp)
add_executable(gml_cli ${_cli})
target_link_libraries(gml_cli
                      PRIVATE gmlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gml/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gml/lsp/main.cpp)
add_library(gmlLsp ${_lsp})
target_link_libraries(gmlLsp PUBLIC gmlCore cursiv::lsp)
add_executable(gml_lsp gml/lsp/main.cpp)
target_link_libraries(gml_lsp PRIVATE gmlLsp)

cursiv_add_benchmark(gml SOURCES gml/bench/main.cpp)

cursiv_package_language(gml)
