# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `plantumlGenerated` (the compiled library) and `plantuml_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID plantuml
    GRAMMAR    plantuml/core/PlantUMLGrammar.hpp
    NAMESPACE  plantuml::PlantUML
    EXTENSIONS .plantuml
    OUT        plantuml/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/core/*.cpp)
add_library(plantumlCore ${_core})
target_link_libraries(plantumlCore PUBLIC plantumlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/cli/*.cpp)
add_executable(plantuml_cli ${_cli})
target_link_libraries(plantuml_cli
                      PRIVATE plantumlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/lsp/main.cpp)
add_library(plantumlLsp ${_lsp})
target_link_libraries(plantumlLsp PUBLIC plantumlCore cursiv::lsp)
add_executable(plantuml_lsp plantuml/lsp/main.cpp)
target_link_libraries(plantuml_lsp PRIVATE plantumlLsp)

cursiv_add_benchmark(plantuml SOURCES plantuml/bench/main.cpp)

cursiv_package_language(plantuml)
