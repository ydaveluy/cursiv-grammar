# Generate the parser and its language module.
cursiv_generate_language(ID corundum
    GRAMMAR    corundum/core/CorundumGrammar.hpp
    NAMESPACE  corundum::Corundum
    EXTENSIONS .rb
    OUT        corundum/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/corundum/core/*.cpp)
add_library(corundumCore ${_core})
target_link_libraries(corundumCore PUBLIC corundumGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/corundum/cli/*.cpp)
if(_cli)
  add_executable(corundum_cli ${_cli})
  target_link_libraries(corundum_cli
                        PRIVATE corundumCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/corundum/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/corundum/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/corundum/lsp/main.cpp)
  add_library(corundumLsp ${_lsp})
  target_link_libraries(corundumLsp PUBLIC corundumCore cursiv::lsp)
  add_executable(corundum_lsp corundum/lsp/main.cpp)
  target_link_libraries(corundum_lsp PRIVATE corundumLsp)
endif()

if(TARGET corundumLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/corundum/bench/main.cpp")
  cursiv_add_benchmark(corundum
      SOURCES corundum/bench/main.cpp)
endif()

cursiv_package_language(corundum)
