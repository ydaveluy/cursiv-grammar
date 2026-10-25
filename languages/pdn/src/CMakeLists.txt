# Generate the parser and its language module.
cursiv_generate_language(ID pdn
    GRAMMAR    pdn/core/PdnGrammar.hpp
    NAMESPACE  pdn::Pdn
    EXTENSIONS .pdn
    OUT        pdn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pdn/core/*.cpp)
add_library(pdnCore ${_core})
target_link_libraries(pdnCore PUBLIC pdnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pdn/cli/*.cpp)
if(_cli)
  add_executable(pdn_cli ${_cli})
  target_link_libraries(pdn_cli
                        PRIVATE pdnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pdn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pdn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pdn/lsp/main.cpp)
  add_library(pdnLsp ${_lsp})
  target_link_libraries(pdnLsp PUBLIC pdnCore cursiv::lsp)
  add_executable(pdn_lsp pdn/lsp/main.cpp)
  target_link_libraries(pdn_lsp PRIVATE pdnLsp)
endif()

if(TARGET pdnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pdn/bench/main.cpp")
  cursiv_add_benchmark(pdn
      SOURCES pdn/bench/main.cpp)
endif()

cursiv_package_language(pdn)
