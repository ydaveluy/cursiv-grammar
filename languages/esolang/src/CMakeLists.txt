# Generate the parser and its language module.
cursiv_generate_language(ID lolcode
    GRAMMAR    lolcode/core/LolcodeGrammar.hpp
    NAMESPACE  lolcode::Lolcode
    EXTENSIONS .lol
    OUT        lolcode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/core/*.cpp)
add_library(lolcodeCore ${_core})
target_link_libraries(lolcodeCore PUBLIC lolcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/cli/*.cpp)
if(_cli)
  add_executable(lolcode_cli ${_cli})
  target_link_libraries(lolcode_cli
                        PRIVATE lolcodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lolcode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/lsp/main.cpp)
  add_library(lolcodeLsp ${_lsp})
  target_link_libraries(lolcodeLsp PUBLIC lolcodeCore cursiv::lsp)
  add_executable(lolcode_lsp lolcode/lsp/main.cpp)
  target_link_libraries(lolcode_lsp PRIVATE lolcodeLsp)
endif()

if(TARGET lolcodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lolcode/bench/main.cpp")
  cursiv_add_benchmark(lolcode
      SOURCES lolcode/bench/main.cpp)
endif()

cursiv_package_language(lolcode)
