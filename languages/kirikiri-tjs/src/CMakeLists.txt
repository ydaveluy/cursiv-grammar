# Generate the parser and its language module.
cursiv_generate_language(ID kirikiritjs
    GRAMMAR    kirikiritjs/core/KirikiriTjsGrammar.hpp
    NAMESPACE  kirikiritjs::KirikiriTjs
    EXTENSIONS .tjs
    OUT        kirikiritjs/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/core/*.cpp)
add_library(kirikiritjsCore ${_core})
target_link_libraries(kirikiritjsCore PUBLIC kirikiritjsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/cli/*.cpp)
if(_cli)
  add_executable(kirikiritjs_cli ${_cli})
  target_link_libraries(kirikiritjs_cli
                        PRIVATE kirikiritjsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/lsp/main.cpp)
  add_library(kirikiritjsLsp ${_lsp})
  target_link_libraries(kirikiritjsLsp PUBLIC kirikiritjsCore cursiv::lsp)
  add_executable(kirikiritjs_lsp kirikiritjs/lsp/main.cpp)
  target_link_libraries(kirikiritjs_lsp PRIVATE kirikiritjsLsp)
endif()

if(TARGET kirikiritjsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kirikiritjs/bench/main.cpp")
  cursiv_add_benchmark(kirikiritjs
      SOURCES kirikiritjs/bench/main.cpp)
endif()

cursiv_package_language(kirikiritjs)
