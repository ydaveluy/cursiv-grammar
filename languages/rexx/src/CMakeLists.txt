# Generate the parser and its language module.
cursiv_generate_language(ID rexx
    GRAMMAR    rexx/core/RexxGrammar.hpp
    NAMESPACE  rexx::Rexx
    EXTENSIONS .rex
    OUT        rexx/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rexx/core/*.cpp)
add_library(rexxCore ${_core})
target_link_libraries(rexxCore PUBLIC rexxGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rexx/cli/*.cpp)
if(_cli)
  add_executable(rexx_cli ${_cli})
  target_link_libraries(rexx_cli
                        PRIVATE rexxCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rexx/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rexx/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rexx/lsp/main.cpp)
  add_library(rexxLsp ${_lsp})
  target_link_libraries(rexxLsp PUBLIC rexxCore cursiv::lsp)
  add_executable(rexx_lsp rexx/lsp/main.cpp)
  target_link_libraries(rexx_lsp PRIVATE rexxLsp)
endif()

if(TARGET rexxLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rexx/bench/main.cpp")
  cursiv_add_benchmark(rexx
      SOURCES rexx/bench/main.cpp)
endif()

cursiv_package_language(rexx)
