# Generate the parser and its language module.
cursiv_generate_language(ID argus
    GRAMMAR    argus/core/ArgusGrammar.hpp
    NAMESPACE  argus::Argus
    EXTENSIONS .arg
    OUT        argus/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/argus/core/*.cpp)
add_library(argusCore ${_core})
target_link_libraries(argusCore PUBLIC argusGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/argus/cli/*.cpp)
if(_cli)
  add_executable(argus_cli ${_cli})
  target_link_libraries(argus_cli
                        PRIVATE argusCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/argus/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/argus/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/argus/lsp/main.cpp)
  add_library(argusLsp ${_lsp})
  target_link_libraries(argusLsp PUBLIC argusCore cursiv::lsp)
  add_executable(argus_lsp argus/lsp/main.cpp)
  target_link_libraries(argus_lsp PRIVATE argusLsp)
endif()

if(TARGET argusLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/argus/bench/main.cpp")
  cursiv_add_benchmark(argus
      SOURCES argus/bench/main.cpp)
endif()

cursiv_package_language(argus)
