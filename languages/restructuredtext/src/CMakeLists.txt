# Generate the parser and its language module.
cursiv_generate_language(ID restructuredtext
    GRAMMAR    restructuredtext/core/RestructuredtextGrammar.hpp
    NAMESPACE  restructuredtext::Restructuredtext
    EXTENSIONS .rst
    OUT        restructuredtext/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/core/*.cpp)
add_library(restructuredtextCore ${_core})
target_link_libraries(restructuredtextCore PUBLIC restructuredtextGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/cli/*.cpp)
if(_cli)
  add_executable(restructuredtext_cli ${_cli})
  target_link_libraries(restructuredtext_cli
                        PRIVATE restructuredtextCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/lsp/main.cpp)
  add_library(restructuredtextLsp ${_lsp})
  target_link_libraries(restructuredtextLsp PUBLIC restructuredtextCore cursiv::lsp)
  add_executable(restructuredtext_lsp restructuredtext/lsp/main.cpp)
  target_link_libraries(restructuredtext_lsp PRIVATE restructuredtextLsp)
endif()

if(TARGET restructuredtextLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/bench/main.cpp")
  cursiv_add_benchmark(restructuredtext
      SOURCES restructuredtext/bench/main.cpp)
endif()

cursiv_package_language(restructuredtext)
