# Generate the parser and its language module.
cursiv_generate_language(ID glsl
    GRAMMAR    glsl/core/GlslGrammar.hpp
    NAMESPACE  glsl::Glsl
    EXTENSIONS .glsl .vert .frag .geom .tesc .tese .comp
               .rgen .rint .rahit .rchit .rmiss .rcall .mesh .task
    OUT        glsl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/glsl/core/*.cpp)
add_library(glslCore ${_core})
target_link_libraries(glslCore PUBLIC glslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/glsl/cli/*.cpp)
if(_cli)
  add_executable(glsl_cli ${_cli})
  target_link_libraries(glsl_cli
                        PRIVATE glslCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/glsl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/glsl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/glsl/lsp/main.cpp)
  add_library(glslLsp ${_lsp})
  target_link_libraries(glslLsp PUBLIC glslCore cursiv::lsp)
  add_executable(glsl_lsp glsl/lsp/main.cpp)
  target_link_libraries(glsl_lsp PRIVATE glslLsp)
endif()

if(TARGET glslLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glsl/bench/main.cpp")
  cursiv_add_benchmark(glsl
      SOURCES glsl/bench/main.cpp)
endif()

cursiv_package_language(glsl)
