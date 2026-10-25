# Generate the parser and its language module.
cursiv_generate_language(ID tinyc
    GRAMMAR    tinyc/core/TinycGrammar.hpp
    NAMESPACE  tinyc::Tinyc
    EXTENSIONS .tinyc
    OUT        tinyc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/core/*.cpp)
add_library(tinycCore ${_core})
target_link_libraries(tinycCore PUBLIC tinycGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/cli/*.cpp)
if(_cli)
  add_executable(tinyc_cli ${_cli})
  target_link_libraries(tinyc_cli
                        PRIVATE tinycCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tinyc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinyc/lsp/main.cpp)
  add_library(tinycLsp ${_lsp})
  target_link_libraries(tinycLsp PUBLIC tinycCore cursiv::lsp)
  add_executable(tinyc_lsp tinyc/lsp/main.cpp)
  target_link_libraries(tinyc_lsp PRIVATE tinycLsp)
endif()

if(TARGET tinycLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinyc/bench/main.cpp")
  cursiv_add_benchmark(tinyc
      SOURCES tinyc/bench/main.cpp)
endif()

cursiv_package_language(tinyc)
