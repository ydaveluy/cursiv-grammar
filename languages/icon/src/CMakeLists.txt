# Generate the parser and its language module.
cursiv_generate_language(ID icon
    GRAMMAR    icon/core/IconGrammar.hpp
    NAMESPACE  icon::Icon
    EXTENSIONS .icn
    OUT        icon/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icon/core/*.cpp)
add_library(iconCore ${_core})
target_link_libraries(iconCore PUBLIC iconGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icon/cli/*.cpp)
if(_cli)
  add_executable(icon_cli ${_cli})
  target_link_libraries(icon_cli
                        PRIVATE iconCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/icon/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/icon/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/icon/lsp/main.cpp)
  add_library(iconLsp ${_lsp})
  target_link_libraries(iconLsp PUBLIC iconCore cursiv::lsp)
  add_executable(icon_lsp icon/lsp/main.cpp)
  target_link_libraries(icon_lsp PRIVATE iconLsp)
endif()

if(TARGET iconLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icon/bench/main.cpp")
  cursiv_add_benchmark(icon
      SOURCES icon/bench/main.cpp)
endif()

cursiv_package_language(icon)
