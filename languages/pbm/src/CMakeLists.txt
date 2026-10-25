# Generate the parser and its language module.
cursiv_generate_language(ID pbm
    GRAMMAR    pbm/core/PbmGrammar.hpp
    NAMESPACE  pbm::Pbm
    EXTENSIONS .pbm
    OUT        pbm/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pbm/core/*.cpp)
add_library(pbmCore ${_core})
target_link_libraries(pbmCore PUBLIC pbmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pbm/cli/*.cpp)
if(_cli)
  add_executable(pbm_cli ${_cli})
  target_link_libraries(pbm_cli
                        PRIVATE pbmCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pbm/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pbm/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pbm/lsp/main.cpp)
  add_library(pbmLsp ${_lsp})
  target_link_libraries(pbmLsp PUBLIC pbmCore cursiv::lsp)
  add_executable(pbm_lsp pbm/lsp/main.cpp)
  target_link_libraries(pbm_lsp PRIVATE pbmLsp)
endif()

if(TARGET pbmLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pbm/bench/main.cpp")
  cursiv_add_benchmark(pbm
      SOURCES pbm/bench/main.cpp)
endif()

cursiv_package_language(pbm)
