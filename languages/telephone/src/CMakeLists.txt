# Generate the parser and its language module.
cursiv_generate_language(ID telephone
    GRAMMAR    telephone/core/TelephoneGrammar.hpp
    NAMESPACE  telephone::Telephone
    EXTENSIONS .tel
    OUT        telephone/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/telephone/core/*.cpp)
add_library(telephoneCore ${_core})
target_link_libraries(telephoneCore PUBLIC telephoneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/telephone/cli/*.cpp)
if(_cli)
  add_executable(telephone_cli ${_cli})
  target_link_libraries(telephone_cli
                        PRIVATE telephoneCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/telephone/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/telephone/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/telephone/lsp/main.cpp)
  add_library(telephoneLsp ${_lsp})
  target_link_libraries(telephoneLsp PUBLIC telephoneCore cursiv::lsp)
  add_executable(telephone_lsp telephone/lsp/main.cpp)
  target_link_libraries(telephone_lsp PRIVATE telephoneLsp)
endif()

if(TARGET telephoneLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/telephone/bench/main.cpp")
  cursiv_add_benchmark(telephone
      SOURCES telephone/bench/main.cpp)
endif()

cursiv_package_language(telephone)
