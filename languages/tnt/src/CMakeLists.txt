# Generate the parser and its language module.
cursiv_generate_language(ID tnt
    GRAMMAR    tnt/core/TntGrammar.hpp
    NAMESPACE  tnt::Tnt
    EXTENSIONS .tnt
    OUT        tnt/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt/core/*.cpp)
add_library(tntCore ${_core})
target_link_libraries(tntCore PUBLIC tntGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnt/cli/*.cpp)
if(_cli)
  add_executable(tnt_cli ${_cli})
  target_link_libraries(tnt_cli
                        PRIVATE tntCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tnt/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tnt/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tnt/lsp/main.cpp)
  add_library(tntLsp ${_lsp})
  target_link_libraries(tntLsp PUBLIC tntCore cursiv::lsp)
  add_executable(tnt_lsp tnt/lsp/main.cpp)
  target_link_libraries(tnt_lsp PRIVATE tntLsp)
endif()

if(TARGET tntLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tnt/bench/main.cpp")
  cursiv_add_benchmark(tnt
      SOURCES tnt/bench/main.cpp)
endif()

cursiv_package_language(tnt)
