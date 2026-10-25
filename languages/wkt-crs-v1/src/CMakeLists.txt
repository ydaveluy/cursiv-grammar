# Generate the parser and its language module.
cursiv_generate_language(ID wktcrsv1
    GRAMMAR    wktcrsv1/core/WktCrsV1Grammar.hpp
    NAMESPACE  wktcrsv1::WktCrsV1
    EXTENSIONS .wktcrsv1
    OUT        wktcrsv1/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/core/*.cpp)
add_library(wktcrsv1Core ${_core})
target_link_libraries(wktcrsv1Core PUBLIC wktcrsv1Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/cli/*.cpp)
if(_cli)
  add_executable(wktcrsv1_cli ${_cli})
  target_link_libraries(wktcrsv1_cli
                        PRIVATE wktcrsv1Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/lsp/main.cpp)
  add_library(wktcrsv1Lsp ${_lsp})
  target_link_libraries(wktcrsv1Lsp PUBLIC wktcrsv1Core cursiv::lsp)
  add_executable(wktcrsv1_lsp wktcrsv1/lsp/main.cpp)
  target_link_libraries(wktcrsv1_lsp PRIVATE wktcrsv1Lsp)
endif()

if(TARGET wktcrsv1Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wktcrsv1/bench/main.cpp")
  cursiv_add_benchmark(wktcrsv1
      SOURCES wktcrsv1/bench/main.cpp)
endif()

cursiv_package_language(wktcrsv1)
