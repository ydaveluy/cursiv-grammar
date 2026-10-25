# Generate the parser and its language module.
cursiv_generate_language(ID janus
    GRAMMAR    janus/core/JanusGrammar.hpp
    NAMESPACE  janus::Janus
    EXTENSIONS .janus
    OUT        janus/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/janus/core/*.cpp)
add_library(janusCore ${_core})
target_link_libraries(janusCore PUBLIC janusGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/janus/cli/*.cpp)
if(_cli)
  add_executable(janus_cli ${_cli})
  target_link_libraries(janus_cli
                        PRIVATE janusCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/janus/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/janus/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/janus/lsp/main.cpp)
  add_library(janusLsp ${_lsp})
  target_link_libraries(janusLsp PUBLIC janusCore cursiv::lsp)
  add_executable(janus_lsp janus/lsp/main.cpp)
  target_link_libraries(janus_lsp PRIVATE janusLsp)
endif()

if(TARGET janusLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/janus/bench/main.cpp")
  cursiv_add_benchmark(janus
      SOURCES janus/bench/main.cpp)
endif()

cursiv_package_language(janus)
