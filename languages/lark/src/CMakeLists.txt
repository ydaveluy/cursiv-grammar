# Generate the parser and its language module.
cursiv_generate_language(ID lark
    GRAMMAR    lark/core/LarkGrammar.hpp
    NAMESPACE  lark::Lark
    EXTENSIONS .lark
    OUT        lark/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lark/core/*.cpp)
add_library(larkCore ${_core})
target_link_libraries(larkCore PUBLIC larkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lark/cli/*.cpp)
if(_cli)
  add_executable(lark_cli ${_cli})
  target_link_libraries(lark_cli
                        PRIVATE larkCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lark/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lark/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lark/lsp/main.cpp)
  add_library(larkLsp ${_lsp})
  target_link_libraries(larkLsp PUBLIC larkCore cursiv::lsp)
  add_executable(lark_lsp lark/lsp/main.cpp)
  target_link_libraries(lark_lsp PRIVATE larkLsp)
endif()

if(TARGET larkLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lark/bench/main.cpp")
  cursiv_add_benchmark(lark
      SOURCES lark/bench/main.cpp)
endif()

cursiv_package_language(lark)
