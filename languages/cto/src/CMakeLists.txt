# Generate the parser and its language module.
cursiv_generate_language(ID cto
    GRAMMAR    cto/core/CtoGrammar.hpp
    NAMESPACE  cto::Cto
    EXTENSIONS .cto
    OUT        cto/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cto/core/*.cpp)
add_library(ctoCore ${_core})
target_link_libraries(ctoCore PUBLIC ctoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cto/cli/*.cpp)
if(_cli)
  add_executable(cto_cli ${_cli})
  target_link_libraries(cto_cli
                        PRIVATE ctoCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cto/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cto/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cto/lsp/main.cpp)
  add_library(ctoLsp ${_lsp})
  target_link_libraries(ctoLsp PUBLIC ctoCore cursiv::lsp)
  add_executable(cto_lsp cto/lsp/main.cpp)
  target_link_libraries(cto_lsp PRIVATE ctoLsp)
endif()

if(TARGET ctoLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cto/bench/main.cpp")
  cursiv_add_benchmark(cto
      SOURCES cto/bench/main.cpp)
endif()

cursiv_package_language(cto)
