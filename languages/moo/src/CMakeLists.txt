# Generate the parser and its language module.
cursiv_generate_language(ID moo
    GRAMMAR    moo/core/MooGrammar.hpp
    NAMESPACE  moo::Moo
    EXTENSIONS .moo
    OUT        moo/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/moo/core/*.cpp)
add_library(mooCore ${_core})
target_link_libraries(mooCore PUBLIC mooGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/moo/cli/*.cpp)
if(_cli)
  add_executable(moo_cli ${_cli})
  target_link_libraries(moo_cli
                        PRIVATE mooCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/moo/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/moo/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/moo/lsp/main.cpp)
  add_library(mooLsp ${_lsp})
  target_link_libraries(mooLsp PUBLIC mooCore cursiv::lsp)
  add_executable(moo_lsp moo/lsp/main.cpp)
  target_link_libraries(moo_lsp PRIVATE mooLsp)
endif()

if(TARGET mooLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/moo/bench/main.cpp")
  cursiv_add_benchmark(moo
      SOURCES moo/bench/main.cpp)
endif()

cursiv_package_language(moo)
