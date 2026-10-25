# Generate the parser and its language module.
cursiv_generate_language(ID cql3
    GRAMMAR    cql3/core/Cql3Grammar.hpp
    NAMESPACE  cql3::Cql3
    EXTENSIONS .cql3
    OUT        cql3/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql3/core/*.cpp)
add_library(cql3Core ${_core})
target_link_libraries(cql3Core PUBLIC cql3Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql3/cli/*.cpp)
if(_cli)
  add_executable(cql3_cli ${_cli})
  target_link_libraries(cql3_cli
                        PRIVATE cql3Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cql3/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cql3/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cql3/lsp/main.cpp)
  add_library(cql3Lsp ${_lsp})
  target_link_libraries(cql3Lsp PUBLIC cql3Core cursiv::lsp)
  add_executable(cql3_lsp cql3/lsp/main.cpp)
  target_link_libraries(cql3_lsp PRIVATE cql3Lsp)
endif()

if(TARGET cql3Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cql3/bench/main.cpp")
  cursiv_add_benchmark(cql3
      SOURCES cql3/bench/main.cpp)
endif()

cursiv_package_language(cql3)
