# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `blueprintGenerated` (the compiled library) and `blueprint_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID blueprint
    GRAMMAR    blueprint/core/BlueprintGrammar.hpp
    NAMESPACE  blueprint::Blueprint
    EXTENSIONS .bp
    OUT        blueprint/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/core/*.cpp)
add_library(blueprintCore ${_core})
target_link_libraries(blueprintCore PUBLIC blueprintGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/cli/*.cpp)
add_executable(blueprint_cli ${_cli})
target_link_libraries(blueprint_cli
                      PRIVATE blueprintCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/lsp/main.cpp)
add_library(blueprintLsp ${_lsp})
target_link_libraries(blueprintLsp PUBLIC blueprintCore cursiv::lsp)
add_executable(blueprint_lsp blueprint/lsp/main.cpp)
target_link_libraries(blueprint_lsp PRIVATE blueprintLsp)

cursiv_add_benchmark(blueprint SOURCES blueprint/bench/main.cpp)

cursiv_package_language(blueprint)
