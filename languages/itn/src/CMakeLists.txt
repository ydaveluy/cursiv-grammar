# Generate the parser and its language module.
cursiv_generate_language(ID itn
    GRAMMAR    itn/core/ItnGrammar.hpp
    NAMESPACE  itn::Itn
    EXTENSIONS .itn
    OUT        itn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/itn/core/*.cpp)
add_library(itnCore ${_core})
target_link_libraries(itnCore PUBLIC itnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/itn/cli/*.cpp)
if(_cli)
  add_executable(itn_cli ${_cli})
  target_link_libraries(itn_cli
                        PRIVATE itnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/itn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/itn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/itn/lsp/main.cpp)
  add_library(itnLsp ${_lsp})
  target_link_libraries(itnLsp PUBLIC itnCore cursiv::lsp)
  add_executable(itn_lsp itn/lsp/main.cpp)
  target_link_libraries(itn_lsp PRIVATE itnLsp)
endif()

if(TARGET itnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/itn/bench/main.cpp")
  cursiv_add_benchmark(itn
      SOURCES itn/bench/main.cpp)
endif()

cursiv_package_language(itn)
