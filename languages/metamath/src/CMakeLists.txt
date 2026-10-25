# Generate the parser and its language module.
cursiv_generate_language(ID metamath
    GRAMMAR    metamath/core/MetamathGrammar.hpp
    NAMESPACE  metamath::Metamath
    EXTENSIONS .mm
    OUT        metamath/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metamath/core/*.cpp)
add_library(metamathCore ${_core})
target_link_libraries(metamathCore PUBLIC metamathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metamath/cli/*.cpp)
if(_cli)
  add_executable(metamath_cli ${_cli})
  target_link_libraries(metamath_cli
                        PRIVATE metamathCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/metamath/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/metamath/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/metamath/lsp/main.cpp)
  add_library(metamathLsp ${_lsp})
  target_link_libraries(metamathLsp PUBLIC metamathCore cursiv::lsp)
  add_executable(metamath_lsp metamath/lsp/main.cpp)
  target_link_libraries(metamath_lsp PRIVATE metamathLsp)
endif()

if(TARGET metamathLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/metamath/bench/main.cpp")
  cursiv_add_benchmark(metamath
      SOURCES metamath/bench/main.cpp)
endif()

cursiv_package_language(metamath)
