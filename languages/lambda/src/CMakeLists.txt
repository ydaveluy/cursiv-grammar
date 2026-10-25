# Generate the parser and its language module.
cursiv_generate_language(ID lambda
    GRAMMAR    lambda/core/LambdaGrammar.hpp
    NAMESPACE  lambda::Lambda
    EXTENSIONS .lambda
    OUT        lambda/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lambda/core/*.cpp)
add_library(lambdaCore ${_core})
target_link_libraries(lambdaCore PUBLIC lambdaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lambda/cli/*.cpp)
if(_cli)
  add_executable(lambda_cli ${_cli})
  target_link_libraries(lambda_cli
                        PRIVATE lambdaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lambda/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lambda/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lambda/lsp/main.cpp)
  add_library(lambdaLsp ${_lsp})
  target_link_libraries(lambdaLsp PUBLIC lambdaCore cursiv::lsp)
  add_executable(lambda_lsp lambda/lsp/main.cpp)
  target_link_libraries(lambda_lsp PRIVATE lambdaLsp)
endif()

if(TARGET lambdaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lambda/bench/main.cpp")
  cursiv_add_benchmark(lambda
      SOURCES lambda/bench/main.cpp)
endif()

cursiv_package_language(lambda)
