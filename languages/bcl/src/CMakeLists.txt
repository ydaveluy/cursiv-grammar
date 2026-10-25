# Generate the parser and its language module.
cursiv_generate_language(ID bcl
    GRAMMAR    bcl/core/BclGrammar.hpp
    NAMESPACE  bcl::Bcl
    EXTENSIONS .bcl
    OUT        bcl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcl/core/*.cpp)
add_library(bclCore ${_core})
target_link_libraries(bclCore PUBLIC bclGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcl/cli/*.cpp)
if(_cli)
  add_executable(bcl_cli ${_cli})
  target_link_libraries(bcl_cli
                        PRIVATE bclCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bcl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bcl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bcl/lsp/main.cpp)
  add_library(bclLsp ${_lsp})
  target_link_libraries(bclLsp PUBLIC bclCore cursiv::lsp)
  add_executable(bcl_lsp bcl/lsp/main.cpp)
  target_link_libraries(bcl_lsp PRIVATE bclLsp)
endif()

if(TARGET bclLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bcl/bench/main.cpp")
  cursiv_add_benchmark(bcl
      SOURCES bcl/bench/main.cpp)
endif()

cursiv_package_language(bcl)
