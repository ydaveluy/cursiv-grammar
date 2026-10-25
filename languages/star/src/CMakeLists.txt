# Generate the parser and its language module.
cursiv_generate_language(ID star
    GRAMMAR    star/core/StarGrammar.hpp
    NAMESPACE  star::Star
    EXTENSIONS .star
    OUT        star/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/star/core/*.cpp)
add_library(starCore ${_core})
target_link_libraries(starCore PUBLIC starGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/star/cli/*.cpp)
if(_cli)
  add_executable(star_cli ${_cli})
  target_link_libraries(star_cli
                        PRIVATE starCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/star/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/star/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/star/lsp/main.cpp)
  add_library(starLsp ${_lsp})
  target_link_libraries(starLsp PUBLIC starCore cursiv::lsp)
  add_executable(star_lsp star/lsp/main.cpp)
  target_link_libraries(star_lsp PRIVATE starLsp)
endif()

if(TARGET starLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/star/bench/main.cpp")
  cursiv_add_benchmark(star
      SOURCES star/bench/main.cpp)
endif()

cursiv_package_language(star)
