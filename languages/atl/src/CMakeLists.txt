# Generate the parser and its language module.
cursiv_generate_language(ID atl
    GRAMMAR    atl/core/AtlGrammar.hpp
    NAMESPACE  atl::Atl
    EXTENSIONS .atl
    OUT        atl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/atl/core/*.cpp)
add_library(atlCore ${_core})
target_link_libraries(atlCore PUBLIC atlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/atl/cli/*.cpp)
if(_cli)
  add_executable(atl_cli ${_cli})
  target_link_libraries(atl_cli
                        PRIVATE atlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/atl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/atl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/atl/lsp/main.cpp)
  add_library(atlLsp ${_lsp})
  target_link_libraries(atlLsp PUBLIC atlCore cursiv::lsp)
  add_executable(atl_lsp atl/lsp/main.cpp)
  target_link_libraries(atl_lsp PRIVATE atlLsp)
endif()

if(TARGET atlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/atl/bench/main.cpp")
  cursiv_add_benchmark(atl
      SOURCES atl/bench/main.cpp)
endif()

cursiv_package_language(atl)
