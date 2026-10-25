# Generate the parser and its language module.
cursiv_generate_language(ID edn
    GRAMMAR    edn/core/EdnGrammar.hpp
    NAMESPACE  edn::Edn
    EXTENSIONS .edn
    OUT        edn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edn/core/*.cpp)
add_library(ednCore ${_core})
target_link_libraries(ednCore PUBLIC ednGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edn/cli/*.cpp)
if(_cli)
  add_executable(edn_cli ${_cli})
  target_link_libraries(edn_cli
                        PRIVATE ednCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/edn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/edn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/edn/lsp/main.cpp)
  add_library(ednLsp ${_lsp})
  target_link_libraries(ednLsp PUBLIC ednCore cursiv::lsp)
  add_executable(edn_lsp edn/lsp/main.cpp)
  target_link_libraries(edn_lsp PRIVATE ednLsp)
endif()

if(TARGET ednLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/edn/bench/main.cpp")
  cursiv_add_benchmark(edn
      SOURCES edn/bench/main.cpp)
endif()

cursiv_package_language(edn)
