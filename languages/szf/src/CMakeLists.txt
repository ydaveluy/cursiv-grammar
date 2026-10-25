# Generate the parser and its language module.
cursiv_generate_language(ID szf
    GRAMMAR    szf/core/SzfGrammar.hpp
    NAMESPACE  szf::Szf
    EXTENSIONS .sfz .szf
    OUT        szf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/szf/core/*.cpp)
add_library(szfCore ${_core})
target_link_libraries(szfCore PUBLIC szfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/szf/cli/*.cpp)
if(_cli)
  add_executable(szf_cli ${_cli})
  target_link_libraries(szf_cli
                        PRIVATE szfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/szf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/szf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/szf/lsp/main.cpp)
  add_library(szfLsp ${_lsp})
  target_link_libraries(szfLsp PUBLIC szfCore cursiv::lsp)
  add_executable(szf_lsp szf/lsp/main.cpp)
  target_link_libraries(szf_lsp PRIVATE szfLsp)
endif()

if(TARGET szfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/szf/bench/main.cpp")
  cursiv_add_benchmark(szf
      SOURCES szf/bench/main.cpp)
endif()

cursiv_package_language(szf)
