# Generate the parser and its language module.
cursiv_generate_language(ID mumps
    GRAMMAR    mumps/core/MumpsGrammar.hpp
    NAMESPACE  mumps::Mumps
    EXTENSIONS .m
    OUT        mumps/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumps/core/*.cpp)
add_library(mumpsCore ${_core})
target_link_libraries(mumpsCore PUBLIC mumpsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumps/cli/*.cpp)
if(_cli)
  add_executable(mumps_cli ${_cli})
  target_link_libraries(mumps_cli
                        PRIVATE mumpsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mumps/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/mumps/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mumps/lsp/main.cpp)
  add_library(mumpsLsp ${_lsp})
  target_link_libraries(mumpsLsp PUBLIC mumpsCore cursiv::lsp)
  add_executable(mumps_lsp mumps/lsp/main.cpp)
  target_link_libraries(mumps_lsp PRIVATE mumpsLsp)
endif()

if(TARGET mumpsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mumps/bench/main.cpp")
  cursiv_add_benchmark(mumps
      SOURCES mumps/bench/main.cpp)
endif()

cursiv_package_language(mumps)
