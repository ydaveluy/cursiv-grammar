# Generate the parser and its language module.
cursiv_generate_language(ID gff3
    GRAMMAR    gff3/core/Gff3Grammar.hpp
    NAMESPACE  gff3::Gff3
    EXTENSIONS .gff
    OUT        gff3/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gff3/core/*.cpp)
add_library(gff3Core ${_core})
target_link_libraries(gff3Core PUBLIC gff3Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gff3/cli/*.cpp)
if(_cli)
  add_executable(gff3_cli ${_cli})
  target_link_libraries(gff3_cli
                        PRIVATE gff3Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gff3/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gff3/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gff3/lsp/main.cpp)
  add_library(gff3Lsp ${_lsp})
  target_link_libraries(gff3Lsp PUBLIC gff3Core cursiv::lsp)
  add_executable(gff3_lsp gff3/lsp/main.cpp)
  target_link_libraries(gff3_lsp PRIVATE gff3Lsp)
endif()

if(TARGET gff3Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gff3/bench/main.cpp")
  cursiv_add_benchmark(gff3
      SOURCES gff3/bench/main.cpp)
endif()

cursiv_package_language(gff3)
