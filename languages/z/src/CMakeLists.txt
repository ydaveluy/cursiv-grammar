# Generate the parser and its language module.
cursiv_generate_language(ID z
    GRAMMAR    z/core/ZGrammar.hpp
    NAMESPACE  z::Z
    EXTENSIONS .zed
    OUT        z/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/z/core/*.cpp)
add_library(zCore ${_core})
target_link_libraries(zCore PUBLIC zGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/z/cli/*.cpp)
if(_cli)
  add_executable(z_cli ${_cli})
  target_link_libraries(z_cli
                        PRIVATE zCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/z/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/z/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/z/lsp/main.cpp)
  add_library(zLsp ${_lsp})
  target_link_libraries(zLsp PUBLIC zCore cursiv::lsp)
  add_executable(z_lsp z/lsp/main.cpp)
  target_link_libraries(z_lsp PRIVATE zLsp)
endif()

if(TARGET zLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/z/bench/main.cpp")
  cursiv_add_benchmark(z
      SOURCES z/bench/main.cpp)
endif()

cursiv_package_language(z)
