# Generate the parser and its language module.
cursiv_generate_language(ID stl
    GRAMMAR    stl/core/StlGrammar.hpp
    NAMESPACE  stl::Stl
    EXTENSIONS .stl
    OUT        stl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stl/core/*.cpp)
add_library(stlCore ${_core})
target_link_libraries(stlCore PUBLIC stlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stl/cli/*.cpp)
if(_cli)
  add_executable(stl_cli ${_cli})
  target_link_libraries(stl_cli
                        PRIVATE stlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/stl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/stl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stl/lsp/main.cpp)
  add_library(stlLsp ${_lsp})
  target_link_libraries(stlLsp PUBLIC stlCore cursiv::lsp)
  add_executable(stl_lsp stl/lsp/main.cpp)
  target_link_libraries(stl_lsp PRIVATE stlLsp)
endif()

if(TARGET stlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stl/bench/main.cpp")
  cursiv_add_benchmark(stl
      SOURCES stl/bench/main.cpp)
endif()

cursiv_package_language(stl)
