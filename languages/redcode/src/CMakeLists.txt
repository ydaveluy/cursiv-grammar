# Generate the parser and its language module.
cursiv_generate_language(ID redcode
    GRAMMAR    redcode/core/RedcodeGrammar.hpp
    NAMESPACE  redcode::Redcode
    EXTENSIONS .redcode
    OUT        redcode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redcode/core/*.cpp)
add_library(redcodeCore ${_core})
target_link_libraries(redcodeCore PUBLIC redcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redcode/cli/*.cpp)
if(_cli)
  add_executable(redcode_cli ${_cli})
  target_link_libraries(redcode_cli
                        PRIVATE redcodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/redcode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/redcode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/redcode/lsp/main.cpp)
  add_library(redcodeLsp ${_lsp})
  target_link_libraries(redcodeLsp PUBLIC redcodeCore cursiv::lsp)
  add_executable(redcode_lsp redcode/lsp/main.cpp)
  target_link_libraries(redcode_lsp PRIVATE redcodeLsp)
endif()

if(TARGET redcodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/redcode/bench/main.cpp")
  cursiv_add_benchmark(redcode
      SOURCES redcode/bench/main.cpp)
endif()

cursiv_package_language(redcode)
