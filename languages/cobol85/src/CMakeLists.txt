# Generate the parser and its language module.
cursiv_generate_language(ID cobol85
    GRAMMAR    cobol85/core/Cobol85Grammar.hpp
    NAMESPACE  cobol85::Cobol85
    EXTENSIONS .cbl
    OUT        cobol85/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/core/*.cpp)
add_library(cobol85Core ${_core})
target_link_libraries(cobol85Core PUBLIC cobol85Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/cli/*.cpp)
if(_cli)
  add_executable(cobol85_cli ${_cli})
  target_link_libraries(cobol85_cli
                        PRIVATE cobol85Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cobol85/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/lsp/main.cpp)
  add_library(cobol85Lsp ${_lsp})
  target_link_libraries(cobol85Lsp PUBLIC cobol85Core cursiv::lsp)
  add_executable(cobol85_lsp cobol85/lsp/main.cpp)
  target_link_libraries(cobol85_lsp PRIVATE cobol85Lsp)
endif()

if(TARGET cobol85Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cobol85/bench/main.cpp")
  cursiv_add_benchmark(cobol85
      SOURCES cobol85/bench/main.cpp)
endif()

cursiv_package_language(cobol85)
