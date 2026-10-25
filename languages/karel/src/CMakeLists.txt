# Generate the parser and its language module.
cursiv_generate_language(ID karel
    GRAMMAR    karel/core/KarelGrammar.hpp
    NAMESPACE  karel::Karel
    EXTENSIONS .karel
    OUT        karel/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/karel/core/*.cpp)
add_library(karelCore ${_core})
target_link_libraries(karelCore PUBLIC karelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/karel/cli/*.cpp)
if(_cli)
  add_executable(karel_cli ${_cli})
  target_link_libraries(karel_cli
                        PRIVATE karelCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/karel/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/karel/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/karel/lsp/main.cpp)
  add_library(karelLsp ${_lsp})
  target_link_libraries(karelLsp PUBLIC karelCore cursiv::lsp)
  add_executable(karel_lsp karel/lsp/main.cpp)
  target_link_libraries(karel_lsp PRIVATE karelLsp)
endif()

if(TARGET karelLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/karel/bench/main.cpp")
  cursiv_add_benchmark(karel
      SOURCES karel/bench/main.cpp)
endif()

cursiv_package_language(karel)
