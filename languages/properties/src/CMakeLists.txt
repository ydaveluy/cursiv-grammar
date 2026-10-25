# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `propertiesGenerated` (the compiled library) and `properties_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID properties
    GRAMMAR    properties/core/PropertiesGrammar.hpp
    NAMESPACE  properties::Properties
    EXTENSIONS .properties
    OUT        properties/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/properties/core/*.cpp)
add_library(propertiesCore ${_core})
target_link_libraries(propertiesCore PUBLIC propertiesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/properties/cli/*.cpp)
add_executable(properties_cli ${_cli})
target_link_libraries(properties_cli
                      PRIVATE propertiesCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/properties/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/properties/lsp/main.cpp)
add_library(propertiesLsp ${_lsp})
target_link_libraries(propertiesLsp PUBLIC propertiesCore cursiv::lsp)
add_executable(properties_lsp properties/lsp/main.cpp)
target_link_libraries(properties_lsp PRIVATE propertiesLsp)

cursiv_add_benchmark(properties SOURCES properties/bench/main.cpp)

cursiv_package_language(properties)
