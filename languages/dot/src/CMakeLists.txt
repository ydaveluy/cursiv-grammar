# Generate the parser and its language module.
cursiv_generate_language(ID dot
    GRAMMAR    dot/core/DotGrammar.hpp
    NAMESPACE  dot::Dot
    EXTENSIONS .dot
    OUT        dot/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dot/core/*.cpp)
add_library(dotCore ${_core})
target_link_libraries(dotCore PUBLIC dotGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dot/cli/*.cpp)
if(_cli)
  add_executable(dot_cli ${_cli})
  target_link_libraries(dot_cli
                        PRIVATE dotCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dot/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dot/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dot/lsp/main.cpp)
  add_library(dotLsp ${_lsp})
  target_link_libraries(dotLsp PUBLIC dotCore cursiv::lsp)
  add_executable(dot_lsp dot/lsp/main.cpp)
  target_link_libraries(dot_lsp PRIVATE dotLsp)
endif()

if(TARGET dotLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dot/bench/main.cpp")
  cursiv_add_benchmark(dot
      SOURCES dot/bench/main.cpp)
endif()

cursiv_package_language(dot)
