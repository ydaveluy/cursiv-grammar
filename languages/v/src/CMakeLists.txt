# Generate the parser and its language module.
cursiv_generate_language(ID v
    GRAMMAR    v/core/VGrammar.hpp
    NAMESPACE  v::V
    EXTENSIONS .v
    OUT        v/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/v/core/*.cpp)
add_library(vCore ${_core})
target_link_libraries(vCore PUBLIC vGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/v/cli/*.cpp)
if(_cli)
  add_executable(v_cli ${_cli})
  target_link_libraries(v_cli
                        PRIVATE vCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/v/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/v/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/v/lsp/main.cpp)
  add_library(vLsp ${_lsp})
  target_link_libraries(vLsp PUBLIC vCore cursiv::lsp)
  add_executable(v_lsp v/lsp/main.cpp)
  target_link_libraries(v_lsp PRIVATE vLsp)
endif()

if(TARGET vLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/v/bench/main.cpp")
  cursiv_add_benchmark(v
      SOURCES v/bench/main.cpp)
endif()

cursiv_package_language(v)
