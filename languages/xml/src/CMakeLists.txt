# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `xmlGenerated` (the compiled library) and `xml_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID xml
    GRAMMAR    xml/core/XmlGrammar.hpp
    NAMESPACE  xml::Xml
    EXTENSIONS .xml
    OUT        xml/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xml/core/*.cpp)
add_library(xmlCore ${_core})
target_link_libraries(xmlCore PUBLIC xmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xml/cli/*.cpp)
add_executable(xml_cli ${_cli})
target_link_libraries(xml_cli
                      PRIVATE xmlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xml/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xml/lsp/main.cpp)
add_library(xmlLsp ${_lsp})
target_link_libraries(xmlLsp PUBLIC xmlCore cursiv::lsp)
add_executable(xml_lsp xml/lsp/main.cpp)
target_link_libraries(xml_lsp PRIVATE xmlLsp)

cursiv_add_benchmark(xml SOURCES xml/bench/main.cpp)

cursiv_package_language(xml)
