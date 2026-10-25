# Generate the parser and its language module.
cursiv_generate_language(ID stellaris
    GRAMMAR    stellaris/core/StellarisGrammar.hpp
    NAMESPACE  stellaris::Stellaris
    EXTENSIONS .stellaris
    OUT        stellaris/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/core/*.cpp)
add_library(stellarisCore ${_core})
target_link_libraries(stellarisCore PUBLIC stellarisGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/cli/*.cpp)
if(_cli)
  add_executable(stellaris_cli ${_cli})
  target_link_libraries(stellaris_cli
                        PRIVATE stellarisCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/stellaris/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stellaris/lsp/main.cpp)
  add_library(stellarisLsp ${_lsp})
  target_link_libraries(stellarisLsp PUBLIC stellarisCore cursiv::lsp)
  add_executable(stellaris_lsp stellaris/lsp/main.cpp)
  target_link_libraries(stellaris_lsp PRIVATE stellarisLsp)
endif()

if(TARGET stellarisLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stellaris/bench/main.cpp")
  cursiv_add_benchmark(stellaris
      SOURCES stellaris/bench/main.cpp)
endif()

cursiv_package_language(stellaris)
