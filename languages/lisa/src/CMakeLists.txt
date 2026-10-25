# Generate the parser and its language module.
cursiv_generate_language(ID lisa
    GRAMMAR    lisa/core/LisaGrammar.hpp
    NAMESPACE  lisa::Lisa
    EXTENSIONS .lisa
    OUT        lisa/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisa/core/*.cpp)
add_library(lisaCore ${_core})
target_link_libraries(lisaCore PUBLIC lisaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisa/cli/*.cpp)
if(_cli)
  add_executable(lisa_cli ${_cli})
  target_link_libraries(lisa_cli
                        PRIVATE lisaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lisa/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lisa/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lisa/lsp/main.cpp)
  add_library(lisaLsp ${_lsp})
  target_link_libraries(lisaLsp PUBLIC lisaCore cursiv::lsp)
  add_executable(lisa_lsp lisa/lsp/main.cpp)
  target_link_libraries(lisa_lsp PRIVATE lisaLsp)
endif()

if(TARGET lisaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lisa/bench/main.cpp")
  cursiv_add_benchmark(lisa
      SOURCES lisa/bench/main.cpp)
endif()

cursiv_package_language(lisa)
