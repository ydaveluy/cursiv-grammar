# Generate the parser and its language module.
cursiv_generate_language(ID d2
    GRAMMAR    d2/core/D2Grammar.hpp
    NAMESPACE  d2::D2
    EXTENSIONS .d2
    OUT        d2/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/d2/core/*.cpp)
add_library(d2Core ${_core})
target_link_libraries(d2Core PUBLIC d2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/d2/cli/*.cpp)
if(_cli)
  add_executable(d2_cli ${_cli})
  target_link_libraries(d2_cli
                        PRIVATE d2Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/d2/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/d2/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/d2/lsp/main.cpp)
  add_library(d2Lsp ${_lsp})
  target_link_libraries(d2Lsp PUBLIC d2Core cursiv::lsp)
  add_executable(d2_lsp d2/lsp/main.cpp)
  target_link_libraries(d2_lsp PRIVATE d2Lsp)
endif()

if(TARGET d2Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/d2/bench/main.cpp")
  cursiv_add_benchmark(d2
      SOURCES d2/bench/main.cpp)
endif()

cursiv_package_language(d2)
