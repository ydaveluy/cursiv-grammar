# Generate the parser and its language module.
cursiv_generate_language(ID sdl
    GRAMMAR    sdl/core/SdlGrammar.hpp
    NAMESPACE  sdl::Sdl
    EXTENSIONS .sdl
    OUT        sdl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sdl/core/*.cpp)
add_library(sdlCore ${_core})
target_link_libraries(sdlCore PUBLIC sdlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sdl/cli/*.cpp)
if(_cli)
  add_executable(sdl_cli ${_cli})
  target_link_libraries(sdl_cli
                        PRIVATE sdlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sdl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sdl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sdl/lsp/main.cpp)
  add_library(sdlLsp ${_lsp})
  target_link_libraries(sdlLsp PUBLIC sdlCore cursiv::lsp)
  add_executable(sdl_lsp sdl/lsp/main.cpp)
  target_link_libraries(sdl_lsp PRIVATE sdlLsp)
endif()

if(TARGET sdlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sdl/bench/main.cpp")
  cursiv_add_benchmark(sdl
      SOURCES sdl/bench/main.cpp)
endif()

cursiv_package_language(sdl)
