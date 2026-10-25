# Generate the parser and its language module.
cursiv_generate_language(ID pest
    GRAMMAR    pest/core/PestGrammar.hpp
    NAMESPACE  pest::Pest
    EXTENSIONS .pest
    OUT        pest/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pest/core/*.cpp)
add_library(pestCore ${_core})
target_link_libraries(pestCore PUBLIC pestGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pest/cli/*.cpp)
if(_cli)
  add_executable(pest_cli ${_cli})
  target_link_libraries(pest_cli
                        PRIVATE pestCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pest/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pest/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pest/lsp/main.cpp)
  add_library(pestLsp ${_lsp})
  target_link_libraries(pestLsp PUBLIC pestCore cursiv::lsp)
  add_executable(pest_lsp pest/lsp/main.cpp)
  target_link_libraries(pest_lsp PRIVATE pestLsp)
endif()

if(TARGET pestLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pest/bench/main.cpp")
  cursiv_add_benchmark(pest
      SOURCES pest/bench/main.cpp)
endif()

cursiv_package_language(pest)
