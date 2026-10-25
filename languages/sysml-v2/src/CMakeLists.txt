# Generate the parser and its language module.
cursiv_generate_language(ID sysmlv2
    GRAMMAR    sysmlv2/core/SysmlV2Grammar.hpp
    NAMESPACE  sysmlv2::SysmlV2
    EXTENSIONS .sysml
    OUT        sysmlv2/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/core/*.cpp)
add_library(sysmlv2Core ${_core})
target_link_libraries(sysmlv2Core PUBLIC sysmlv2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/cli/*.cpp)
if(_cli)
  add_executable(sysmlv2_cli ${_cli})
  target_link_libraries(sysmlv2_cli
                        PRIVATE sysmlv2Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/lsp/main.cpp)
  add_library(sysmlv2Lsp ${_lsp})
  target_link_libraries(sysmlv2Lsp PUBLIC sysmlv2Core cursiv::lsp)
  add_executable(sysmlv2_lsp sysmlv2/lsp/main.cpp)
  target_link_libraries(sysmlv2_lsp PRIVATE sysmlv2Lsp)
endif()

if(TARGET sysmlv2Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sysmlv2/bench/main.cpp")
  cursiv_add_benchmark(sysmlv2
      SOURCES sysmlv2/bench/main.cpp)
endif()

cursiv_package_language(sysmlv2)
