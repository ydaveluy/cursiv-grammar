# Generate the parser and its language module.
cursiv_generate_language(ID xyz
    GRAMMAR    xyz/core/XyzGrammar.hpp
    NAMESPACE  xyz::Xyz
    EXTENSIONS .xyz
    OUT        xyz/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xyz/core/*.cpp)
add_library(xyzCore ${_core})
target_link_libraries(xyzCore PUBLIC xyzGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xyz/cli/*.cpp)
if(_cli)
  add_executable(xyz_cli ${_cli})
  target_link_libraries(xyz_cli
                        PRIVATE xyzCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xyz/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/xyz/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xyz/lsp/main.cpp)
  add_library(xyzLsp ${_lsp})
  target_link_libraries(xyzLsp PUBLIC xyzCore cursiv::lsp)
  add_executable(xyz_lsp xyz/lsp/main.cpp)
  target_link_libraries(xyz_lsp PRIVATE xyzLsp)
endif()

if(TARGET xyzLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xyz/bench/main.cpp")
  cursiv_add_benchmark(xyz
      SOURCES xyz/bench/main.cpp)
endif()

cursiv_package_language(xyz)
