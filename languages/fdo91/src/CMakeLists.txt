# Generate the parser and its language module.
cursiv_generate_language(ID fdo91
    GRAMMAR    fdo91/core/Fdo91Grammar.hpp
    NAMESPACE  fdo91::Fdo91
    EXTENSIONS .fdo91
    OUT        fdo91/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/core/*.cpp)
add_library(fdo91Core ${_core})
target_link_libraries(fdo91Core PUBLIC fdo91Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/cli/*.cpp)
if(_cli)
  add_executable(fdo91_cli ${_cli})
  target_link_libraries(fdo91_cli
                        PRIVATE fdo91Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fdo91/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fdo91/lsp/main.cpp)
  add_library(fdo91Lsp ${_lsp})
  target_link_libraries(fdo91Lsp PUBLIC fdo91Core cursiv::lsp)
  add_executable(fdo91_lsp fdo91/lsp/main.cpp)
  target_link_libraries(fdo91_lsp PRIVATE fdo91Lsp)
endif()

if(TARGET fdo91Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fdo91/bench/main.cpp")
  cursiv_add_benchmark(fdo91
      SOURCES fdo91/bench/main.cpp)
endif()

cursiv_package_language(fdo91)
