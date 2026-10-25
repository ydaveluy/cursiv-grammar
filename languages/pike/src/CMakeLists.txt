# Generate the parser and its language module.
cursiv_generate_language(ID pike
    GRAMMAR    pike/core/PikeGrammar.hpp
    NAMESPACE  pike::Pike
    EXTENSIONS .pike
    OUT        pike/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pike/core/*.cpp)
add_library(pikeCore ${_core})
target_link_libraries(pikeCore PUBLIC pikeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pike/cli/*.cpp)
if(_cli)
  add_executable(pike_cli ${_cli})
  target_link_libraries(pike_cli
                        PRIVATE pikeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pike/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pike/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pike/lsp/main.cpp)
  add_library(pikeLsp ${_lsp})
  target_link_libraries(pikeLsp PUBLIC pikeCore cursiv::lsp)
  add_executable(pike_lsp pike/lsp/main.cpp)
  target_link_libraries(pike_lsp PRIVATE pikeLsp)
endif()

if(TARGET pikeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pike/bench/main.cpp")
  cursiv_add_benchmark(pike
      SOURCES pike/bench/main.cpp)
endif()

cursiv_package_language(pike)
