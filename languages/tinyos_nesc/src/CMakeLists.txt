# Generate the parser and its language module.
cursiv_generate_language(ID tinyosnesc
    GRAMMAR    tinyosnesc/core/TinyosNescGrammar.hpp
    NAMESPACE  tinyosnesc::TinyosNesc
    EXTENSIONS .nc
    OUT        tinyosnesc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/core/*.cpp)
add_library(tinyosnescCore ${_core})
target_link_libraries(tinyosnescCore PUBLIC tinyosnescGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/cli/*.cpp)
if(_cli)
  add_executable(tinyosnesc_cli ${_cli})
  target_link_libraries(tinyosnesc_cli
                        PRIVATE tinyosnescCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/lsp/main.cpp)
  add_library(tinyosnescLsp ${_lsp})
  target_link_libraries(tinyosnescLsp PUBLIC tinyosnescCore cursiv::lsp)
  add_executable(tinyosnesc_lsp tinyosnesc/lsp/main.cpp)
  target_link_libraries(tinyosnesc_lsp PRIVATE tinyosnescLsp)
endif()

if(TARGET tinyosnescLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/bench/main.cpp")
  cursiv_add_benchmark(tinyosnesc
      SOURCES tinyosnesc/bench/main.cpp)
endif()

cursiv_package_language(tinyosnesc)
