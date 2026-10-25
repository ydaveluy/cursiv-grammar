# Generate the parser and its language module.
cursiv_generate_language(ID pmmn
    GRAMMAR    pmmn/core/PmmnGrammar.hpp
    NAMESPACE  pmmn::Pmmn
    EXTENSIONS .pmmn
    OUT        pmmn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/core/*.cpp)
add_library(pmmnCore ${_core})
target_link_libraries(pmmnCore PUBLIC pmmnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/cli/*.cpp)
if(_cli)
  add_executable(pmmn_cli ${_cli})
  target_link_libraries(pmmn_cli
                        PRIVATE pmmnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pmmn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/lsp/main.cpp)
  add_library(pmmnLsp ${_lsp})
  target_link_libraries(pmmnLsp PUBLIC pmmnCore cursiv::lsp)
  add_executable(pmmn_lsp pmmn/lsp/main.cpp)
  target_link_libraries(pmmn_lsp PRIVATE pmmnLsp)
endif()

if(TARGET pmmnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pmmn/bench/main.cpp")
  cursiv_add_benchmark(pmmn
      SOURCES pmmn/bench/main.cpp)
endif()

cursiv_package_language(pmmn)
