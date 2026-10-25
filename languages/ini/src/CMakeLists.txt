# Generate the parser and its language module.
cursiv_generate_language(ID         ini
    GRAMMAR    ini/core/IniGrammar.hpp
    NAMESPACE  ini::INI
    EXTENSIONS .ini
    OUT        ini/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ini/core/*.cpp)
add_library(iniCore ${_core})
target_link_libraries(iniCore PUBLIC iniGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ini/cli/*.cpp)
if(_cli)
  add_executable(ini_cli ${_cli})
  target_link_libraries(ini_cli
                        PRIVATE iniCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ini/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ini/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ini/lsp/main.cpp)
  add_library(iniLsp ${_lsp})
  target_link_libraries(iniLsp PUBLIC iniCore cursiv::lsp)
  add_executable(ini_lsp ini/lsp/main.cpp)
  target_link_libraries(ini_lsp PRIVATE iniLsp)
endif()

if(TARGET iniLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ini/bench/main.cpp")
  cursiv_add_benchmark(ini
      SOURCES ini/bench/main.cpp)
endif()

cursiv_package_language(ini)
