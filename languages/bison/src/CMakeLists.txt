# Generate the parser and its language module.
cursiv_generate_language(ID bison
    GRAMMAR    bison/core/BisonGrammar.hpp
    NAMESPACE  bison::Bison
    EXTENSIONS .y
    OUT        bison/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bison/core/*.cpp)
add_library(bisonCore ${_core})
target_link_libraries(bisonCore PUBLIC bisonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bison/cli/*.cpp)
if(_cli)
  add_executable(bison_cli ${_cli})
  target_link_libraries(bison_cli
                        PRIVATE bisonCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bison/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bison/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bison/lsp/main.cpp)
  add_library(bisonLsp ${_lsp})
  target_link_libraries(bisonLsp PUBLIC bisonCore cursiv::lsp)
  add_executable(bison_lsp bison/lsp/main.cpp)
  target_link_libraries(bison_lsp PRIVATE bisonLsp)
endif()

if(TARGET bisonLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bison/bench/main.cpp")
  cursiv_add_benchmark(bison
      SOURCES bison/bench/main.cpp)
endif()

cursiv_package_language(bison)
