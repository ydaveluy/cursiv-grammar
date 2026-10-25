# Generate the parser and its language module.
cursiv_generate_language(ID parkingsign
    GRAMMAR    parkingsign/core/ParkingsignGrammar.hpp
    NAMESPACE  parkingsign::Parkingsign
    EXTENSIONS .parkingsign
    OUT        parkingsign/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/core/*.cpp)
add_library(parkingsignCore ${_core})
target_link_libraries(parkingsignCore PUBLIC parkingsignGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/cli/*.cpp)
if(_cli)
  add_executable(parkingsign_cli ${_cli})
  target_link_libraries(parkingsign_cli
                        PRIVATE parkingsignCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/lsp/main.cpp)
  add_library(parkingsignLsp ${_lsp})
  target_link_libraries(parkingsignLsp PUBLIC parkingsignCore cursiv::lsp)
  add_executable(parkingsign_lsp parkingsign/lsp/main.cpp)
  target_link_libraries(parkingsign_lsp PRIVATE parkingsignLsp)
endif()

if(TARGET parkingsignLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/bench/main.cpp")
  cursiv_add_benchmark(parkingsign
      SOURCES parkingsign/bench/main.cpp)
endif()

cursiv_package_language(parkingsign)
