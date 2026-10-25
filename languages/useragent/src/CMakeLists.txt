# Generate the parser and its language module.
cursiv_generate_language(ID useragent
    GRAMMAR    useragent/core/UseragentGrammar.hpp
    NAMESPACE  useragent::Useragent
    EXTENSIONS .useragent
    OUT        useragent/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/useragent/core/*.cpp)
add_library(useragentCore ${_core})
target_link_libraries(useragentCore PUBLIC useragentGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/useragent/cli/*.cpp)
if(_cli)
  add_executable(useragent_cli ${_cli})
  target_link_libraries(useragent_cli
                        PRIVATE useragentCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/useragent/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/useragent/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/useragent/lsp/main.cpp)
  add_library(useragentLsp ${_lsp})
  target_link_libraries(useragentLsp PUBLIC useragentCore cursiv::lsp)
  add_executable(useragent_lsp useragent/lsp/main.cpp)
  target_link_libraries(useragent_lsp PRIVATE useragentLsp)
endif()

if(TARGET useragentLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/useragent/bench/main.cpp")
  cursiv_add_benchmark(useragent
      SOURCES useragent/bench/main.cpp)
endif()

cursiv_package_language(useragent)
