# Generate the parser and its language module.
cursiv_generate_language(ID rpn
    GRAMMAR    rpn/core/RpnGrammar.hpp
    NAMESPACE  rpn::Rpn
    EXTENSIONS .rpn
    OUT        rpn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rpn/core/*.cpp)
add_library(rpnCore ${_core})
target_link_libraries(rpnCore PUBLIC rpnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rpn/cli/*.cpp)
if(_cli)
  add_executable(rpn_cli ${_cli})
  target_link_libraries(rpn_cli
                        PRIVATE rpnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rpn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rpn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rpn/lsp/main.cpp)
  add_library(rpnLsp ${_lsp})
  target_link_libraries(rpnLsp PUBLIC rpnCore cursiv::lsp)
  add_executable(rpn_lsp rpn/lsp/main.cpp)
  target_link_libraries(rpn_lsp PRIVATE rpnLsp)
endif()

if(TARGET rpnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpn/bench/main.cpp")
  cursiv_add_benchmark(rpn
      SOURCES rpn/bench/main.cpp)
endif()

cursiv_package_language(rpn)
