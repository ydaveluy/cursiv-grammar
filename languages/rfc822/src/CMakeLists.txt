# Generate the parser and its language module.
cursiv_generate_language(ID rfc822
    GRAMMAR    rfc822/core/Rfc822Grammar.hpp
    NAMESPACE  rfc822::Rfc822
    EXTENSIONS .rfc822
    OUT        rfc822/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/core/*.cpp)
add_library(rfc822Core ${_core})
target_link_libraries(rfc822Core PUBLIC rfc822Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/cli/*.cpp)
if(_cli)
  add_executable(rfc822_cli ${_cli})
  target_link_libraries(rfc822_cli
                        PRIVATE rfc822Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rfc822/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc822/lsp/main.cpp)
  add_library(rfc822Lsp ${_lsp})
  target_link_libraries(rfc822Lsp PUBLIC rfc822Core cursiv::lsp)
  add_executable(rfc822_lsp rfc822/lsp/main.cpp)
  target_link_libraries(rfc822_lsp PRIVATE rfc822Lsp)
endif()

if(TARGET rfc822Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rfc822/bench/main.cpp")
  cursiv_add_benchmark(rfc822
      SOURCES rfc822/bench/main.cpp)
endif()

cursiv_package_language(rfc822)
