# Generate the parser and its language module.
cursiv_generate_language(ID focal
    GRAMMAR    focal/core/FocalGrammar.hpp
    NAMESPACE  focal::Focal
    EXTENSIONS .focal
    OUT        focal/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/focal/core/*.cpp)
add_library(focalCore ${_core})
target_link_libraries(focalCore PUBLIC focalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/focal/cli/*.cpp)
if(_cli)
  add_executable(focal_cli ${_cli})
  target_link_libraries(focal_cli
                        PRIVATE focalCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/focal/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/focal/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/focal/lsp/main.cpp)
  add_library(focalLsp ${_lsp})
  target_link_libraries(focalLsp PUBLIC focalCore cursiv::lsp)
  add_executable(focal_lsp focal/lsp/main.cpp)
  target_link_libraries(focal_lsp PRIVATE focalLsp)
endif()

if(TARGET focalLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/focal/bench/main.cpp")
  cursiv_add_benchmark(focal
      SOURCES focal/bench/main.cpp)
endif()

cursiv_package_language(focal)
