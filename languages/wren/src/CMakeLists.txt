# Generate the parser and its language module.
cursiv_generate_language(ID wren
    GRAMMAR    wren/core/WrenGrammar.hpp
    NAMESPACE  wren::Wren
    EXTENSIONS .wren
    OUT        wren/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wren/core/*.cpp)
add_library(wrenCore ${_core})
target_link_libraries(wrenCore PUBLIC wrenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wren/cli/*.cpp)
if(_cli)
  add_executable(wren_cli ${_cli})
  target_link_libraries(wren_cli
                        PRIVATE wrenCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wren/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wren/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wren/lsp/main.cpp)
  add_library(wrenLsp ${_lsp})
  target_link_libraries(wrenLsp PUBLIC wrenCore cursiv::lsp)
  add_executable(wren_lsp wren/lsp/main.cpp)
  target_link_libraries(wren_lsp PRIVATE wrenLsp)
endif()

if(TARGET wrenLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wren/bench/main.cpp")
  cursiv_add_benchmark(wren
      SOURCES wren/bench/main.cpp)
endif()

cursiv_package_language(wren)
