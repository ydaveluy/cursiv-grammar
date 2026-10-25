# Generate the parser and its language module.
cursiv_generate_language(ID jam
    GRAMMAR    jam/core/JamGrammar.hpp
    NAMESPACE  jam::Jam
    EXTENSIONS .jam
    OUT        jam/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jam/core/*.cpp)
add_library(jamCore ${_core})
target_link_libraries(jamCore PUBLIC jamGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jam/cli/*.cpp)
if(_cli)
  add_executable(jam_cli ${_cli})
  target_link_libraries(jam_cli
                        PRIVATE jamCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/jam/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/jam/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/jam/lsp/main.cpp)
  add_library(jamLsp ${_lsp})
  target_link_libraries(jamLsp PUBLIC jamCore cursiv::lsp)
  add_executable(jam_lsp jam/lsp/main.cpp)
  target_link_libraries(jam_lsp PRIVATE jamLsp)
endif()

if(TARGET jamLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jam/bench/main.cpp")
  cursiv_add_benchmark(jam
      SOURCES jam/bench/main.cpp)
endif()

cursiv_package_language(jam)
