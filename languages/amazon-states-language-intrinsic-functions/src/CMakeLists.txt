# Generate the parser and its language module.
cursiv_generate_language(ID amazonstateslanguageintrinsicfunctions
    GRAMMAR    amazonstateslanguageintrinsicfunctions/core/AmazonStatesLanguageIntrinsicFunctionsGrammar.hpp
    NAMESPACE  amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctions
    EXTENSIONS .asl-intrinsic
    OUT        amazonstateslanguageintrinsicfunctions/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/core/*.cpp)
add_library(amazonstateslanguageintrinsicfunctionsCore ${_core})
target_link_libraries(amazonstateslanguageintrinsicfunctionsCore PUBLIC amazonstateslanguageintrinsicfunctionsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/cli/*.cpp)
if(_cli)
  add_executable(amazonstateslanguageintrinsicfunctions_cli ${_cli})
  target_link_libraries(amazonstateslanguageintrinsicfunctions_cli
                        PRIVATE amazonstateslanguageintrinsicfunctionsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/lsp/main.cpp)
  add_library(amazonstateslanguageintrinsicfunctionsLsp ${_lsp})
  target_link_libraries(amazonstateslanguageintrinsicfunctionsLsp PUBLIC amazonstateslanguageintrinsicfunctionsCore cursiv::lsp)
  add_executable(amazonstateslanguageintrinsicfunctions_lsp amazonstateslanguageintrinsicfunctions/lsp/main.cpp)
  target_link_libraries(amazonstateslanguageintrinsicfunctions_lsp PRIVATE amazonstateslanguageintrinsicfunctionsLsp)
endif()

if(TARGET amazonstateslanguageintrinsicfunctionsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/bench/main.cpp")
  cursiv_add_benchmark(amazonstateslanguageintrinsicfunctions
      SOURCES amazonstateslanguageintrinsicfunctions/bench/main.cpp)
endif()

cursiv_package_language(amazonstateslanguageintrinsicfunctions)
