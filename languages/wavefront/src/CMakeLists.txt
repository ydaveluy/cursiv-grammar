# Generate the parser and its language module.
cursiv_generate_language(ID wavefront
    GRAMMAR    wavefront/core/WavefrontGrammar.hpp
    NAMESPACE  wavefront::Wavefront
    EXTENSIONS .obj
    OUT        wavefront/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/core/*.cpp)
add_library(wavefrontCore ${_core})
target_link_libraries(wavefrontCore PUBLIC wavefrontGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/cli/*.cpp)
if(_cli)
  add_executable(wavefront_cli ${_cli})
  target_link_libraries(wavefront_cli
                        PRIVATE wavefrontCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wavefront/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/lsp/main.cpp)
  add_library(wavefrontLsp ${_lsp})
  target_link_libraries(wavefrontLsp PUBLIC wavefrontCore cursiv::lsp)
  add_executable(wavefront_lsp wavefront/lsp/main.cpp)
  target_link_libraries(wavefront_lsp PRIVATE wavefrontLsp)
endif()

if(TARGET wavefrontLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wavefront/bench/main.cpp")
  cursiv_add_benchmark(wavefront
      SOURCES wavefront/bench/main.cpp)
endif()

cursiv_package_language(wavefront)
