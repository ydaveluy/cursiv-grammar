# Generate the parser and its language module.
cursiv_generate_language(ID tiny
    GRAMMAR    tiny/core/TinyGrammar.hpp
    NAMESPACE  tiny::Tiny
    EXTENSIONS .tiny
    OUT        tiny/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tiny/core/*.cpp)
add_library(tinyCore ${_core})
target_link_libraries(tinyCore PUBLIC tinyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tiny/cli/*.cpp)
if(_cli)
  add_executable(tiny_cli ${_cli})
  target_link_libraries(tiny_cli
                        PRIVATE tinyCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tiny/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tiny/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tiny/lsp/main.cpp)
  add_library(tinyLsp ${_lsp})
  target_link_libraries(tinyLsp PUBLIC tinyCore cursiv::lsp)
  add_executable(tiny_lsp tiny/lsp/main.cpp)
  target_link_libraries(tiny_lsp PRIVATE tinyLsp)
endif()

if(TARGET tinyLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tiny/bench/main.cpp")
  cursiv_add_benchmark(tiny
      SOURCES tiny/bench/main.cpp)
endif()

cursiv_package_language(tiny)
