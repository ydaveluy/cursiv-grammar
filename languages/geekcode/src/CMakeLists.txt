# Generate the parser and its language module.
cursiv_generate_language(ID geekcode
    GRAMMAR    geekcode/core/GeekcodeGrammar.hpp
    NAMESPACE  geekcode::Geekcode
    EXTENSIONS .geek
    OUT        geekcode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/core/*.cpp)
add_library(geekcodeCore ${_core})
target_link_libraries(geekcodeCore PUBLIC geekcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/cli/*.cpp)
if(_cli)
  add_executable(geekcode_cli ${_cli})
  target_link_libraries(geekcode_cli
                        PRIVATE geekcodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/geekcode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/lsp/main.cpp)
  add_library(geekcodeLsp ${_lsp})
  target_link_libraries(geekcodeLsp PUBLIC geekcodeCore cursiv::lsp)
  add_executable(geekcode_lsp geekcode/lsp/main.cpp)
  target_link_libraries(geekcode_lsp PRIVATE geekcodeLsp)
endif()

if(TARGET geekcodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geekcode/bench/main.cpp")
  cursiv_add_benchmark(geekcode
      SOURCES geekcode/bench/main.cpp)
endif()

cursiv_package_language(geekcode)
