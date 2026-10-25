# Generate the parser and its language module.
cursiv_generate_language(ID iri
    GRAMMAR    iri/core/IriGrammar.hpp
    NAMESPACE  iri::Iri
    EXTENSIONS .iri
    OUT        iri/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iri/core/*.cpp)
add_library(iriCore ${_core})
target_link_libraries(iriCore PUBLIC iriGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iri/cli/*.cpp)
if(_cli)
  add_executable(iri_cli ${_cli})
  target_link_libraries(iri_cli
                        PRIVATE iriCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/iri/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/iri/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/iri/lsp/main.cpp)
  add_library(iriLsp ${_lsp})
  target_link_libraries(iriLsp PUBLIC iriCore cursiv::lsp)
  add_executable(iri_lsp iri/lsp/main.cpp)
  target_link_libraries(iri_lsp PRIVATE iriLsp)
endif()

if(TARGET iriLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/iri/bench/main.cpp")
  cursiv_add_benchmark(iri
      SOURCES iri/bench/main.cpp)
endif()

cursiv_package_language(iri)
