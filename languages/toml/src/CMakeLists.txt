# Generate the parser and its language module.
cursiv_generate_language(ID toml
    GRAMMAR    toml/core/TomlGrammar.hpp
    NAMESPACE  toml::Toml
    EXTENSIONS .toml
    OUT        toml/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/toml/core/*.cpp)
add_library(tomlCore ${_core})
target_link_libraries(tomlCore PUBLIC tomlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/toml/cli/*.cpp)
if(_cli)
  add_executable(toml_cli ${_cli})
  target_link_libraries(toml_cli
                        PRIVATE tomlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/toml/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/toml/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/toml/lsp/main.cpp)
  add_library(tomlLsp ${_lsp})
  target_link_libraries(tomlLsp PUBLIC tomlCore cursiv::lsp)
  add_executable(toml_lsp toml/lsp/main.cpp)
  target_link_libraries(toml_lsp PRIVATE tomlLsp)
endif()

if(TARGET tomlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toml/bench/main.cpp")
  cursiv_add_benchmark(toml
      SOURCES toml/bench/main.cpp)
endif()

cursiv_package_language(toml)
