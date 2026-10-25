# Generate the parser and its language module.
cursiv_generate_language(ID spass
    GRAMMAR    spass/core/SpassGrammar.hpp
    NAMESPACE  spass::Spass
    EXTENSIONS .spass
    OUT        spass/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spass/core/*.cpp)
add_library(spassCore ${_core})
target_link_libraries(spassCore PUBLIC spassGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spass/cli/*.cpp)
if(_cli)
  add_executable(spass_cli ${_cli})
  target_link_libraries(spass_cli
                        PRIVATE spassCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/spass/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/spass/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/spass/lsp/main.cpp)
  add_library(spassLsp ${_lsp})
  target_link_libraries(spassLsp PUBLIC spassCore cursiv::lsp)
  add_executable(spass_lsp spass/lsp/main.cpp)
  target_link_libraries(spass_lsp PRIVATE spassLsp)
endif()

if(TARGET spassLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spass/bench/main.cpp")
  cursiv_add_benchmark(spass
      SOURCES spass/bench/main.cpp)
endif()

cursiv_package_language(spass)
