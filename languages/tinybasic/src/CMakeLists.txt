# Generate the parser and its language module.
cursiv_generate_language(ID tinybasic
    GRAMMAR    tinybasic/core/TinybasicGrammar.hpp
    NAMESPACE  tinybasic::Tinybasic
    EXTENSIONS .tinybasic
    OUT        tinybasic/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/core/*.cpp)
add_library(tinybasicCore ${_core})
target_link_libraries(tinybasicCore PUBLIC tinybasicGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/cli/*.cpp)
if(_cli)
  add_executable(tinybasic_cli ${_cli})
  target_link_libraries(tinybasic_cli
                        PRIVATE tinybasicCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/lsp/main.cpp)
  add_library(tinybasicLsp ${_lsp})
  target_link_libraries(tinybasicLsp PUBLIC tinybasicCore cursiv::lsp)
  add_executable(tinybasic_lsp tinybasic/lsp/main.cpp)
  target_link_libraries(tinybasic_lsp PRIVATE tinybasicLsp)
endif()

if(TARGET tinybasicLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/bench/main.cpp")
  cursiv_add_benchmark(tinybasic
      SOURCES tinybasic/bench/main.cpp)
endif()

cursiv_package_language(tinybasic)
