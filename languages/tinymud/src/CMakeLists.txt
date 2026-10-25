# Generate the parser and its language module.
cursiv_generate_language(ID tinymud
    GRAMMAR    tinymud/core/TinymudGrammar.hpp
    NAMESPACE  tinymud::Tinymud
    EXTENSIONS .tinymud
    OUT        tinymud/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/core/*.cpp)
add_library(tinymudCore ${_core})
target_link_libraries(tinymudCore PUBLIC tinymudGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/cli/*.cpp)
if(_cli)
  add_executable(tinymud_cli ${_cli})
  target_link_libraries(tinymud_cli
                        PRIVATE tinymudCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tinymud/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/lsp/main.cpp)
  add_library(tinymudLsp ${_lsp})
  target_link_libraries(tinymudLsp PUBLIC tinymudCore cursiv::lsp)
  add_executable(tinymud_lsp tinymud/lsp/main.cpp)
  target_link_libraries(tinymud_lsp PRIVATE tinymudLsp)
endif()

if(TARGET tinymudLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinymud/bench/main.cpp")
  cursiv_add_benchmark(tinymud
      SOURCES tinymud/bench/main.cpp)
endif()

cursiv_package_language(tinymud)
