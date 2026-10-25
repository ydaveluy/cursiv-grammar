# Generate the parser and its language module.
cursiv_generate_language(ID dart2
    GRAMMAR    dart2/core/Dart2Grammar.hpp
    NAMESPACE  dart2::Dart2
    EXTENSIONS .dart
    OUT        dart2/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dart2/core/*.cpp)
add_library(dart2Core ${_core})
target_link_libraries(dart2Core PUBLIC dart2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dart2/cli/*.cpp)
if(_cli)
  add_executable(dart2_cli ${_cli})
  target_link_libraries(dart2_cli
                        PRIVATE dart2Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dart2/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dart2/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dart2/lsp/main.cpp)
  add_library(dart2Lsp ${_lsp})
  target_link_libraries(dart2Lsp PUBLIC dart2Core cursiv::lsp)
  add_executable(dart2_lsp dart2/lsp/main.cpp)
  target_link_libraries(dart2_lsp PRIVATE dart2Lsp)
endif()

if(TARGET dart2Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dart2/bench/main.cpp")
  cursiv_add_benchmark(dart2
      SOURCES dart2/bench/main.cpp)
endif()

cursiv_package_language(dart2)
