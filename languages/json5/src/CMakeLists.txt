# Generate the parser and its language module.
cursiv_generate_language(ID json5
    GRAMMAR    json5/core/Json5Grammar.hpp
    NAMESPACE  json5::Json5
    EXTENSIONS .json5
    OUT        json5/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json5/core/*.cpp)
add_library(json5Core ${_core})
target_link_libraries(json5Core PUBLIC json5Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json5/cli/*.cpp)
if(_cli)
  add_executable(json5_cli ${_cli})
  target_link_libraries(json5_cli
                        PRIVATE json5Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/json5/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/json5/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/json5/lsp/main.cpp)
  add_library(json5Lsp ${_lsp})
  target_link_libraries(json5Lsp PUBLIC json5Core cursiv::lsp)
  add_executable(json5_lsp json5/lsp/main.cpp)
  target_link_libraries(json5_lsp PRIVATE json5Lsp)
endif()

if(TARGET json5Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json5/bench/main.cpp")
  cursiv_add_benchmark(json5
      SOURCES json5/bench/main.cpp)
endif()

cursiv_package_language(json5)
