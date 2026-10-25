# Generate the parser and its language module.
cursiv_generate_language(ID postalcode
    GRAMMAR    postalcode/core/PostalcodeGrammar.hpp
    NAMESPACE  postalcode::Postalcode
    EXTENSIONS .postalcode
    OUT        postalcode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/core/*.cpp)
add_library(postalcodeCore ${_core})
target_link_libraries(postalcodeCore PUBLIC postalcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/cli/*.cpp)
if(_cli)
  add_executable(postalcode_cli ${_cli})
  target_link_libraries(postalcode_cli
                        PRIVATE postalcodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/postalcode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/postalcode/lsp/main.cpp)
  add_library(postalcodeLsp ${_lsp})
  target_link_libraries(postalcodeLsp PUBLIC postalcodeCore cursiv::lsp)
  add_executable(postalcode_lsp postalcode/lsp/main.cpp)
  target_link_libraries(postalcode_lsp PRIVATE postalcodeLsp)
endif()

if(TARGET postalcodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postalcode/bench/main.cpp")
  cursiv_add_benchmark(postalcode
      SOURCES postalcode/bench/main.cpp)
endif()

cursiv_package_language(postalcode)
