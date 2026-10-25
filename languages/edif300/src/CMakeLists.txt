# Generate the parser and its language module.
cursiv_generate_language(ID edif300
    GRAMMAR    edif300/core/Edif300Grammar.hpp
    NAMESPACE  edif300::Edif300
    EXTENSIONS .edf
    OUT        edif300/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edif300/core/*.cpp)
add_library(edif300Core ${_core})
target_link_libraries(edif300Core PUBLIC edif300Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edif300/cli/*.cpp)
if(_cli)
  add_executable(edif300_cli ${_cli})
  target_link_libraries(edif300_cli
                        PRIVATE edif300Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/edif300/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/edif300/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/edif300/lsp/main.cpp)
  add_library(edif300Lsp ${_lsp})
  target_link_libraries(edif300Lsp PUBLIC edif300Core cursiv::lsp)
  add_executable(edif300_lsp edif300/lsp/main.cpp)
  target_link_libraries(edif300_lsp PRIVATE edif300Lsp)
endif()

if(TARGET edif300Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/edif300/bench/main.cpp")
  cursiv_add_benchmark(edif300
      SOURCES edif300/bench/main.cpp)
endif()

cursiv_package_language(edif300)
