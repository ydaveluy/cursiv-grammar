# Generate the parser and its language module.
cursiv_generate_language(ID b
    GRAMMAR    b/core/BGrammar.hpp
    NAMESPACE  b::B
    EXTENSIONS .b
    OUT        b/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/b/core/*.cpp)
add_library(bCore ${_core})
target_link_libraries(bCore PUBLIC bGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/b/cli/*.cpp)
if(_cli)
  add_executable(b_cli ${_cli})
  target_link_libraries(b_cli
                        PRIVATE bCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/b/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/b/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/b/lsp/main.cpp)
  add_library(bLsp ${_lsp})
  target_link_libraries(bLsp PUBLIC bCore cursiv::lsp)
  add_executable(b_lsp b/lsp/main.cpp)
  target_link_libraries(b_lsp PRIVATE bLsp)
endif()

if(TARGET bLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/b/bench/main.cpp")
  cursiv_add_benchmark(b
      SOURCES b/bench/main.cpp)
endif()

cursiv_package_language(b)
