# Generate the parser and its language module.
cursiv_generate_language(ID cmake
    GRAMMAR    cmake/core/CmakeGrammar.hpp
    NAMESPACE  cmake::Cmake
    EXTENSIONS .cmake
    OUT        cmake/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/core/*.cpp)
add_library(cmakeCore ${_core})
target_link_libraries(cmakeCore PUBLIC cmakeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli/*.cpp)
if(_cli)
  add_executable(cmake_cli ${_cli})
  target_link_libraries(cmake_cli
                        PRIVATE cmakeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp/main.cpp)
  add_library(cmakeLsp ${_lsp})
  target_link_libraries(cmakeLsp PUBLIC cmakeCore cursiv::lsp)
  add_executable(cmake_lsp cmake/lsp/main.cpp)
  target_link_libraries(cmake_lsp PRIVATE cmakeLsp)
endif()

if(TARGET cmakeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bench/main.cpp")
  cursiv_add_benchmark(cmake
      SOURCES cmake/bench/main.cpp)
endif()

cursiv_package_language(cmake)
