# Generate the parser and its language module.
cursiv_generate_language(ID vb6
    GRAMMAR    vb6/core/Vb6Grammar.hpp
    NAMESPACE  vb6::Vb6
    EXTENSIONS .bas .cls .frm
    OUT        vb6/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vb6/core/*.cpp)
add_library(vb6Core ${_core})
target_link_libraries(vb6Core PUBLIC vb6Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vb6/cli/*.cpp)
if(_cli)
  add_executable(vb6_cli ${_cli})
  target_link_libraries(vb6_cli
                        PRIVATE vb6Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vb6/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/vb6/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vb6/lsp/main.cpp)
  add_library(vb6Lsp ${_lsp})
  target_link_libraries(vb6Lsp PUBLIC vb6Core cursiv::lsp)
  add_executable(vb6_lsp vb6/lsp/main.cpp)
  target_link_libraries(vb6_lsp PRIVATE vb6Lsp)
endif()

if(TARGET vb6Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vb6/bench/main.cpp")
  cursiv_add_benchmark(vb6
      SOURCES vb6/bench/main.cpp)
endif()

cursiv_package_language(vb6)
