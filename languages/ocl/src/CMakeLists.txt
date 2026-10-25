# Generate the parser and its language module.
cursiv_generate_language(ID ocl
    GRAMMAR    ocl/core/OclGrammar.hpp
    NAMESPACE  ocl::Ocl
    EXTENSIONS .ocl
    OUT        ocl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ocl/core/*.cpp)
add_library(oclCore ${_core})
target_link_libraries(oclCore PUBLIC oclGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ocl/cli/*.cpp)
if(_cli)
  add_executable(ocl_cli ${_cli})
  target_link_libraries(ocl_cli
                        PRIVATE oclCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ocl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ocl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ocl/lsp/main.cpp)
  add_library(oclLsp ${_lsp})
  target_link_libraries(oclLsp PUBLIC oclCore cursiv::lsp)
  add_executable(ocl_lsp ocl/lsp/main.cpp)
  target_link_libraries(ocl_lsp PRIVATE oclLsp)
endif()

if(TARGET oclLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ocl/bench/main.cpp")
  cursiv_add_benchmark(ocl
      SOURCES ocl/bench/main.cpp)
endif()

cursiv_package_language(ocl)
