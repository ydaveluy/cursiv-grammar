# Generate the parser and its language module.
cursiv_generate_language(ID peoplecode
    GRAMMAR    peoplecode/core/PeoplecodeGrammar.hpp
    NAMESPACE  peoplecode::Peoplecode
    EXTENSIONS .pc
    OUT        peoplecode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/core/*.cpp)
add_library(peoplecodeCore ${_core})
target_link_libraries(peoplecodeCore PUBLIC peoplecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/cli/*.cpp)
if(_cli)
  add_executable(peoplecode_cli ${_cli})
  target_link_libraries(peoplecode_cli
                        PRIVATE peoplecodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/lsp/main.cpp)
  add_library(peoplecodeLsp ${_lsp})
  target_link_libraries(peoplecodeLsp PUBLIC peoplecodeCore cursiv::lsp)
  add_executable(peoplecode_lsp peoplecode/lsp/main.cpp)
  target_link_libraries(peoplecode_lsp PRIVATE peoplecodeLsp)
endif()

if(TARGET peoplecodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/bench/main.cpp")
  cursiv_add_benchmark(peoplecode
      SOURCES peoplecode/bench/main.cpp)
endif()

cursiv_package_language(peoplecode)
