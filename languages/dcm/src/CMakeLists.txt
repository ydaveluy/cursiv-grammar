# Generate the parser and its language module.
cursiv_generate_language(ID dcm
    GRAMMAR    dcm/core/DcmGrammar.hpp
    NAMESPACE  dcm::Dcm
    EXTENSIONS .dcm
    OUT        dcm/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dcm/core/*.cpp)
add_library(dcmCore ${_core})
target_link_libraries(dcmCore PUBLIC dcmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dcm/cli/*.cpp)
if(_cli)
  add_executable(dcm_cli ${_cli})
  target_link_libraries(dcm_cli
                        PRIVATE dcmCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dcm/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dcm/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dcm/lsp/main.cpp)
  add_library(dcmLsp ${_lsp})
  target_link_libraries(dcmLsp PUBLIC dcmCore cursiv::lsp)
  add_executable(dcm_lsp dcm/lsp/main.cpp)
  target_link_libraries(dcm_lsp PRIVATE dcmLsp)
endif()

if(TARGET dcmLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dcm/bench/main.cpp")
  cursiv_add_benchmark(dcm
      SOURCES dcm/bench/main.cpp)
endif()

cursiv_package_language(dcm)
