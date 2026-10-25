# Generate the parser and its language module.
cursiv_generate_language(ID php
    GRAMMAR    php/core/PhpGrammar.hpp
    NAMESPACE  php::Php
    EXTENSIONS .php
    OUT        php/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/php/core/*.cpp)
add_library(phpCore ${_core})
target_link_libraries(phpCore PUBLIC phpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/php/cli/*.cpp)
if(_cli)
  add_executable(php_cli ${_cli})
  target_link_libraries(php_cli
                        PRIVATE phpCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/php/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/php/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/php/lsp/main.cpp)
  add_library(phpLsp ${_lsp})
  target_link_libraries(phpLsp PUBLIC phpCore cursiv::lsp)
  add_executable(php_lsp php/lsp/main.cpp)
  target_link_libraries(php_lsp PRIVATE phpLsp)
endif()

if(TARGET phpLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/php/bench/main.cpp")
  cursiv_add_benchmark(php
      SOURCES php/bench/main.cpp)
endif()

cursiv_package_language(php)
