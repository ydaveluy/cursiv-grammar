# Generate the parser and its language module.
cursiv_generate_language(ID pii
    GRAMMAR    pii/core/PiiGrammar.hpp
    NAMESPACE  pii::Pii
    EXTENSIONS .pii
    OUT        pii/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pii/core/*.cpp)
add_library(piiCore ${_core})
target_link_libraries(piiCore PUBLIC piiGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pii/cli/*.cpp)
if(_cli)
  add_executable(pii_cli ${_cli})
  target_link_libraries(pii_cli
                        PRIVATE piiCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pii/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pii/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pii/lsp/main.cpp)
  add_library(piiLsp ${_lsp})
  target_link_libraries(piiLsp PUBLIC piiCore cursiv::lsp)
  add_executable(pii_lsp pii/lsp/main.cpp)
  target_link_libraries(pii_lsp PRIVATE piiLsp)
endif()

if(TARGET piiLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pii/bench/main.cpp")
  cursiv_add_benchmark(pii
      SOURCES pii/bench/main.cpp)
endif()

cursiv_package_language(pii)
