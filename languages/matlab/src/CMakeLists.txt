# Generate the parser and its language module.
cursiv_generate_language(ID matlab
    GRAMMAR    matlab/core/MatlabGrammar.hpp
    NAMESPACE  matlab::Matlab
    EXTENSIONS .matlab
    OUT        matlab/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/matlab/core/*.cpp)
add_library(matlabCore ${_core})
target_link_libraries(matlabCore PUBLIC matlabGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/matlab/cli/*.cpp)
if(_cli)
  add_executable(matlab_cli ${_cli})
  target_link_libraries(matlab_cli
                        PRIVATE matlabCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/matlab/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/matlab/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/matlab/lsp/main.cpp)
  add_library(matlabLsp ${_lsp})
  target_link_libraries(matlabLsp PUBLIC matlabCore cursiv::lsp)
  add_executable(matlab_lsp matlab/lsp/main.cpp)
  target_link_libraries(matlab_lsp PRIVATE matlabLsp)
endif()

if(TARGET matlabLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/matlab/bench/main.cpp")
  cursiv_add_benchmark(matlab
      SOURCES matlab/bench/main.cpp)
endif()

cursiv_package_language(matlab)
