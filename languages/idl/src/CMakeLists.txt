# Generate the parser and its language module.
cursiv_generate_language(ID idl
    GRAMMAR    idl/core/IdlGrammar.hpp
    NAMESPACE  idl::Idl
    EXTENSIONS .idl
    OUT        idl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/idl/core/*.cpp)
add_library(idlCore ${_core})
target_link_libraries(idlCore PUBLIC idlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/idl/cli/*.cpp)
if(_cli)
  add_executable(idl_cli ${_cli})
  target_link_libraries(idl_cli
                        PRIVATE idlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/idl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/idl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/idl/lsp/main.cpp)
  add_library(idlLsp ${_lsp})
  target_link_libraries(idlLsp PUBLIC idlCore cursiv::lsp)
  add_executable(idl_lsp idl/lsp/main.cpp)
  target_link_libraries(idl_lsp PRIVATE idlLsp)
endif()

if(TARGET idlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/idl/bench/main.cpp")
  cursiv_add_benchmark(idl
      SOURCES idl/bench/main.cpp)
endif()

cursiv_package_language(idl)
