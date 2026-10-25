# Generate the parser and its language module.
cursiv_generate_language(ID guido
    GRAMMAR    guido/core/GuidoGrammar.hpp
    NAMESPACE  guido::Guido
    EXTENSIONS .gmn
    OUT        guido/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guido/core/*.cpp)
add_library(guidoCore ${_core})
target_link_libraries(guidoCore PUBLIC guidoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guido/cli/*.cpp)
if(_cli)
  add_executable(guido_cli ${_cli})
  target_link_libraries(guido_cli
                        PRIVATE guidoCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/guido/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/guido/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/guido/lsp/main.cpp)
  add_library(guidoLsp ${_lsp})
  target_link_libraries(guidoLsp PUBLIC guidoCore cursiv::lsp)
  add_executable(guido_lsp guido/lsp/main.cpp)
  target_link_libraries(guido_lsp PRIVATE guidoLsp)
endif()

if(TARGET guidoLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/guido/bench/main.cpp")
  cursiv_add_benchmark(guido
      SOURCES guido/bench/main.cpp)
endif()

cursiv_package_language(guido)
