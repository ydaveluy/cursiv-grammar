# Generate the parser and its language module.
cursiv_generate_language(ID rfc1035
    GRAMMAR    rfc1035/core/Rfc1035Grammar.hpp
    NAMESPACE  rfc1035::Rfc1035
    EXTENSIONS .rfc1035
    OUT        rfc1035/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/core/*.cpp)
add_library(rfc1035Core ${_core})
target_link_libraries(rfc1035Core PUBLIC rfc1035Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/cli/*.cpp)
if(_cli)
  add_executable(rfc1035_cli ${_cli})
  target_link_libraries(rfc1035_cli
                        PRIVATE rfc1035Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/lsp/main.cpp)
  add_library(rfc1035Lsp ${_lsp})
  target_link_libraries(rfc1035Lsp PUBLIC rfc1035Core cursiv::lsp)
  add_executable(rfc1035_lsp rfc1035/lsp/main.cpp)
  target_link_libraries(rfc1035_lsp PRIVATE rfc1035Lsp)
endif()

if(TARGET rfc1035Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rfc1035/bench/main.cpp")
  cursiv_add_benchmark(rfc1035
      SOURCES rfc1035/bench/main.cpp)
endif()

cursiv_package_language(rfc1035)
