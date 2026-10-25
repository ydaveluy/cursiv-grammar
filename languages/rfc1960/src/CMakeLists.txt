# Generate the parser and its language module.
cursiv_generate_language(ID rfc1960
    GRAMMAR    rfc1960/core/Rfc1960Grammar.hpp
    NAMESPACE  rfc1960::Rfc1960
    EXTENSIONS .rfc1960
    OUT        rfc1960/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/core/*.cpp)
add_library(rfc1960Core ${_core})
target_link_libraries(rfc1960Core PUBLIC rfc1960Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/cli/*.cpp)
if(_cli)
  add_executable(rfc1960_cli ${_cli})
  target_link_libraries(rfc1960_cli
                        PRIVATE rfc1960Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/lsp/main.cpp)
  add_library(rfc1960Lsp ${_lsp})
  target_link_libraries(rfc1960Lsp PUBLIC rfc1960Core cursiv::lsp)
  add_executable(rfc1960_lsp rfc1960/lsp/main.cpp)
  target_link_libraries(rfc1960_lsp PRIVATE rfc1960Lsp)
endif()

if(TARGET rfc1960Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rfc1960/bench/main.cpp")
  cursiv_add_benchmark(rfc1960
      SOURCES rfc1960/bench/main.cpp)
endif()

cursiv_package_language(rfc1960)
