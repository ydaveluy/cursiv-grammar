# Generate the parser and its language module.
cursiv_generate_language(ID ply
    GRAMMAR    ply/core/PlyGrammar.hpp
    NAMESPACE  ply::Ply
    EXTENSIONS .ply
    OUT        ply/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ply/core/*.cpp)
add_library(plyCore ${_core})
target_link_libraries(plyCore PUBLIC plyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ply/cli/*.cpp)
if(_cli)
  add_executable(ply_cli ${_cli})
  target_link_libraries(ply_cli
                        PRIVATE plyCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ply/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ply/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ply/lsp/main.cpp)
  add_library(plyLsp ${_lsp})
  target_link_libraries(plyLsp PUBLIC plyCore cursiv::lsp)
  add_executable(ply_lsp ply/lsp/main.cpp)
  target_link_libraries(ply_lsp PRIVATE plyLsp)
endif()

if(TARGET plyLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ply/bench/main.cpp")
  cursiv_add_benchmark(ply
      SOURCES ply/bench/main.cpp)
endif()

cursiv_package_language(ply)
