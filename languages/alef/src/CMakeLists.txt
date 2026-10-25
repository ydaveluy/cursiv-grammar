# Generate the parser and its language module.
cursiv_generate_language(ID alef
    GRAMMAR    alef/core/AlefGrammar.hpp
    NAMESPACE  alef::Alef
    EXTENSIONS .alef
    OUT        alef/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alef/core/*.cpp)
add_library(alefCore ${_core})
target_link_libraries(alefCore PUBLIC alefGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alef/cli/*.cpp)
if(_cli)
  add_executable(alef_cli ${_cli})
  target_link_libraries(alef_cli
                        PRIVATE alefCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/alef/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/alef/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alef/lsp/main.cpp)
  add_library(alefLsp ${_lsp})
  target_link_libraries(alefLsp PUBLIC alefCore cursiv::lsp)
  add_executable(alef_lsp alef/lsp/main.cpp)
  target_link_libraries(alef_lsp PRIVATE alefLsp)
endif()

if(TARGET alefLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/alef/bench/main.cpp")
  cursiv_add_benchmark(alef
      SOURCES alef/bench/main.cpp)
endif()

cursiv_package_language(alef)
