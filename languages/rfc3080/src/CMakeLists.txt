# Generate the parser and its language module.
cursiv_generate_language(ID rfc3080
    GRAMMAR    rfc3080/core/Rfc3080Grammar.hpp
    NAMESPACE  rfc3080::Rfc3080
    EXTENSIONS .beep
    OUT        rfc3080/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/core/*.cpp)
add_library(rfc3080Core ${_core})
target_link_libraries(rfc3080Core PUBLIC rfc3080Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/cli/*.cpp)
if(_cli)
  add_executable(rfc3080_cli ${_cli})
  target_link_libraries(rfc3080_cli
                        PRIVATE rfc3080Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/lsp/main.cpp)
  add_library(rfc3080Lsp ${_lsp})
  target_link_libraries(rfc3080Lsp PUBLIC rfc3080Core cursiv::lsp)
  add_executable(rfc3080_lsp rfc3080/lsp/main.cpp)
  target_link_libraries(rfc3080_lsp PRIVATE rfc3080Lsp)
endif()

if(TARGET rfc3080Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/bench/main.cpp")
  cursiv_add_benchmark(rfc3080
      SOURCES rfc3080/bench/main.cpp)
endif()

cursiv_package_language(rfc3080)
