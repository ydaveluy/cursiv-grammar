# Generate the parser and its language module.
cursiv_generate_language(ID logo
    GRAMMAR    logo/core/LogoGrammar.hpp
    NAMESPACE  logo::Logo
    EXTENSIONS .logo
    OUT        logo/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/logo/core/*.cpp)
add_library(logoCore ${_core})
target_link_libraries(logoCore PUBLIC logoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/logo/cli/*.cpp)
if(_cli)
  add_executable(logo_cli ${_cli})
  target_link_libraries(logo_cli
                        PRIVATE logoCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/logo/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/logo/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/logo/lsp/main.cpp)
  add_library(logoLsp ${_lsp})
  target_link_libraries(logoLsp PUBLIC logoCore cursiv::lsp)
  add_executable(logo_lsp logo/lsp/main.cpp)
  target_link_libraries(logo_lsp PRIVATE logoLsp)
endif()

if(TARGET logoLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/logo/bench/main.cpp")
  cursiv_add_benchmark(logo
      SOURCES logo/bench/main.cpp)
endif()

cursiv_package_language(logo)
