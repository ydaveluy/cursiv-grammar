# Generate the parser and its language module.
cursiv_generate_language(ID iso8601
    GRAMMAR    iso8601/core/Iso8601Grammar.hpp
    NAMESPACE  iso8601::Iso8601
    EXTENSIONS .iso8601
    OUT        iso8601/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/core/*.cpp)
add_library(iso8601Core ${_core})
target_link_libraries(iso8601Core PUBLIC iso8601Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/cli/*.cpp)
if(_cli)
  add_executable(iso8601_cli ${_cli})
  target_link_libraries(iso8601_cli
                        PRIVATE iso8601Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/iso8601/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/lsp/main.cpp)
  add_library(iso8601Lsp ${_lsp})
  target_link_libraries(iso8601Lsp PUBLIC iso8601Core cursiv::lsp)
  add_executable(iso8601_lsp iso8601/lsp/main.cpp)
  target_link_libraries(iso8601_lsp PRIVATE iso8601Lsp)
endif()

if(TARGET iso8601Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/iso8601/bench/main.cpp")
  cursiv_add_benchmark(iso8601
      SOURCES iso8601/bench/main.cpp)
endif()

cursiv_package_language(iso8601)
