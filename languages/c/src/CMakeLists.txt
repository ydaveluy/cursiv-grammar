# Generate the parser and its language module.
cursiv_generate_language(ID c
    GRAMMAR    c/core/CGrammar.hpp
    NAMESPACE  c::C
    EXTENSIONS .c .h
    OUT        c/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/c/core/*.cpp)
add_library(cCore ${_core})
target_link_libraries(cCore PUBLIC cGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/c/cli/*.cpp)
if(_cli)
  add_executable(c_cli ${_cli})
  target_link_libraries(c_cli
                        PRIVATE cCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/c/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/c/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/c/lsp/main.cpp)
  add_library(cLsp ${_lsp})
  target_link_libraries(cLsp PUBLIC cCore cursiv::lsp)
  add_executable(c_lsp c/lsp/main.cpp)
  target_link_libraries(c_lsp PRIVATE cLsp)
endif()

if(TARGET cLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/c/bench/main.cpp")
  cursiv_add_benchmark(c
      SOURCES c/bench/main.cpp)
endif()

cursiv_package_language(c)
