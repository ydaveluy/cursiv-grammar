# Generate the parser and its language module.
cursiv_generate_language(ID maml
    GRAMMAR    maml/core/MamlGrammar.hpp
    NAMESPACE  maml::Maml
    EXTENSIONS .maml
    OUT        maml/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/maml/core/*.cpp)
add_library(mamlCore ${_core})
target_link_libraries(mamlCore PUBLIC mamlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/maml/cli/*.cpp)
if(_cli)
  add_executable(maml_cli ${_cli})
  target_link_libraries(maml_cli
                        PRIVATE mamlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/maml/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/maml/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/maml/lsp/main.cpp)
  add_library(mamlLsp ${_lsp})
  target_link_libraries(mamlLsp PUBLIC mamlCore cursiv::lsp)
  add_executable(maml_lsp maml/lsp/main.cpp)
  target_link_libraries(maml_lsp PRIVATE mamlLsp)
endif()

if(TARGET mamlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maml/bench/main.cpp")
  cursiv_add_benchmark(maml
      SOURCES maml/bench/main.cpp)
endif()

cursiv_package_language(maml)
