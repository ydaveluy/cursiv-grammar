# Generate the parser and its language module.
cursiv_generate_language(ID abb
    GRAMMAR    abb/core/AbbGrammar.hpp
    NAMESPACE  abb::Abb
    EXTENSIONS .sys
    OUT        abb/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abb/core/*.cpp)
add_library(abbCore ${_core})
target_link_libraries(abbCore PUBLIC abbGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abb/cli/*.cpp)
if(_cli)
  add_executable(abb_cli ${_cli})
  target_link_libraries(abb_cli
                        PRIVATE abbCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/abb/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/abb/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/abb/lsp/main.cpp)
  add_library(abbLsp ${_lsp})
  target_link_libraries(abbLsp PUBLIC abbCore cursiv::lsp)
  add_executable(abb_lsp abb/lsp/main.cpp)
  target_link_libraries(abb_lsp PRIVATE abbLsp)
endif()

if(TARGET abbLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/abb/bench/main.cpp")
  cursiv_add_benchmark(abb
      SOURCES abb/bench/main.cpp)
endif()

cursiv_package_language(abb)
