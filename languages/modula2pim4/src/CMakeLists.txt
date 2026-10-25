# Generate the parser and its language module.
cursiv_generate_language(ID modula2pim4
    GRAMMAR    modula2pim4/core/Modula2pim4Grammar.hpp
    NAMESPACE  modula2pim4::Modula2pim4
    EXTENSIONS .mod
    OUT        modula2pim4/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/core/*.cpp)
add_library(modula2pim4Core ${_core})
target_link_libraries(modula2pim4Core PUBLIC modula2pim4Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/cli/*.cpp)
if(_cli)
  add_executable(modula2pim4_cli ${_cli})
  target_link_libraries(modula2pim4_cli
                        PRIVATE modula2pim4Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/lsp/main.cpp)
  add_library(modula2pim4Lsp ${_lsp})
  target_link_libraries(modula2pim4Lsp PUBLIC modula2pim4Core cursiv::lsp)
  add_executable(modula2pim4_lsp modula2pim4/lsp/main.cpp)
  target_link_libraries(modula2pim4_lsp PRIVATE modula2pim4Lsp)
endif()

if(TARGET modula2pim4Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modula2pim4/bench/main.cpp")
  cursiv_add_benchmark(modula2pim4
      SOURCES modula2pim4/bench/main.cpp)
endif()

cursiv_package_language(modula2pim4)
