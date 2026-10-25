# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tomlGenerated` (the compiled library) and `toml_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID toml
    GRAMMAR    toml/core/TomlGrammar.hpp
    NAMESPACE  toml::Toml
    EXTENSIONS .toml
    OUT        toml/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/toml/core/*.cpp)
add_library(tomlCore ${_core})
target_link_libraries(tomlCore PUBLIC tomlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/toml/cli/*.cpp)
add_executable(toml_cli ${_cli})
target_link_libraries(toml_cli
                      PRIVATE tomlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/toml/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/toml/lsp/main.cpp)
add_library(tomlLsp ${_lsp})
target_link_libraries(tomlLsp PUBLIC tomlCore cursiv::lsp)
add_executable(toml_lsp toml/lsp/main.cpp)
target_link_libraries(toml_lsp PRIVATE tomlLsp)

cursiv_add_benchmark(toml SOURCES toml/bench/main.cpp)

cursiv_package_language(toml)
