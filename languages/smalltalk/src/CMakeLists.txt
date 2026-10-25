# Generate the parser and its language module.
cursiv_generate_language(ID smalltalk
    GRAMMAR    smalltalk/core/SmalltalkGrammar.hpp
    NAMESPACE  smalltalk::Smalltalk
    EXTENSIONS .st
    OUT        smalltalk/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/core/*.cpp)
add_library(smalltalkCore ${_core})
target_link_libraries(smalltalkCore PUBLIC smalltalkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/cli/*.cpp)
if(_cli)
  add_executable(smalltalk_cli ${_cli})
  target_link_libraries(smalltalk_cli
                        PRIVATE smalltalkCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/lsp/main.cpp)
  add_library(smalltalkLsp ${_lsp})
  target_link_libraries(smalltalkLsp PUBLIC smalltalkCore cursiv::lsp)
  add_executable(smalltalk_lsp smalltalk/lsp/main.cpp)
  target_link_libraries(smalltalk_lsp PRIVATE smalltalkLsp)
endif()

if(TARGET smalltalkLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/bench/main.cpp")
  cursiv_add_benchmark(smalltalk
      SOURCES smalltalk/bench/main.cpp)
endif()

cursiv_package_language(smalltalk)
