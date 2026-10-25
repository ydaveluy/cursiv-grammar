# Generate the parser and its language module.
cursiv_generate_language(ID apt
    GRAMMAR    apt/core/AptGrammar.hpp
    NAMESPACE  apt::Apt
    EXTENSIONS .list
    OUT        apt/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apt/core/*.cpp)
add_library(aptCore ${_core})
target_link_libraries(aptCore PUBLIC aptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apt/cli/*.cpp)
if(_cli)
  add_executable(apt_cli ${_cli})
  target_link_libraries(apt_cli
                        PRIVATE aptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/apt/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/apt/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/apt/lsp/main.cpp)
  add_library(aptLsp ${_lsp})
  target_link_libraries(aptLsp PUBLIC aptCore cursiv::lsp)
  add_executable(apt_lsp apt/lsp/main.cpp)
  target_link_libraries(apt_lsp PRIVATE aptLsp)
endif()

if(TARGET aptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apt/bench/main.cpp")
  cursiv_add_benchmark(apt
      SOURCES apt/bench/main.cpp)
endif()

cursiv_package_language(apt)
