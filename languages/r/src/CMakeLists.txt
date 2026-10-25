# Generate the parser and its language module.
cursiv_generate_language(ID r
    GRAMMAR    r/core/RGrammar.hpp
    NAMESPACE  r::R
    EXTENSIONS .R
    OUT        r/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/r/core/*.cpp)
add_library(rCore ${_core})
target_link_libraries(rCore PUBLIC rGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/r/cli/*.cpp)
if(_cli)
  add_executable(r_cli ${_cli})
  target_link_libraries(r_cli
                        PRIVATE rCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/r/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/r/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/r/lsp/main.cpp)
  add_library(rLsp ${_lsp})
  target_link_libraries(rLsp PUBLIC rCore cursiv::lsp)
  add_executable(r_lsp r/lsp/main.cpp)
  target_link_libraries(r_lsp PRIVATE rLsp)
endif()

if(TARGET rLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/r/bench/main.cpp")
  cursiv_add_benchmark(r
      SOURCES r/bench/main.cpp)
endif()

cursiv_package_language(r)
