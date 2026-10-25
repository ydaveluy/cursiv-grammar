# Generate the parser and its language module.
cursiv_generate_language(ID fortran
    GRAMMAR    fortran/core/FortranGrammar.hpp
    NAMESPACE  fortran::Fortran
    EXTENSIONS .f90
    OUT        fortran/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fortran/core/*.cpp)
add_library(fortranCore ${_core})
target_link_libraries(fortranCore PUBLIC fortranGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fortran/cli/*.cpp)
if(_cli)
  add_executable(fortran_cli ${_cli})
  target_link_libraries(fortran_cli
                        PRIVATE fortranCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fortran/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/fortran/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fortran/lsp/main.cpp)
  add_library(fortranLsp ${_lsp})
  target_link_libraries(fortranLsp PUBLIC fortranCore cursiv::lsp)
  add_executable(fortran_lsp fortran/lsp/main.cpp)
  target_link_libraries(fortran_lsp PRIVATE fortranLsp)
endif()

if(TARGET fortranLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fortran/bench/main.cpp")
  cursiv_add_benchmark(fortran
      SOURCES fortran/bench/main.cpp)
endif()

cursiv_package_language(fortran)
