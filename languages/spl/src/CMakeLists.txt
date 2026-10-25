# Generate the parser and its language module.
cursiv_generate_language(ID spl
    GRAMMAR    spl/core/SplGrammar.hpp
    NAMESPACE  spl::Spl
    EXTENSIONS .spl
    OUT        spl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spl/core/*.cpp)
add_library(splCore ${_core})
target_link_libraries(splCore PUBLIC splGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spl/cli/*.cpp)
if(_cli)
  add_executable(spl_cli ${_cli})
  target_link_libraries(spl_cli
                        PRIVATE splCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/spl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/spl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/spl/lsp/main.cpp)
  add_library(splLsp ${_lsp})
  target_link_libraries(splLsp PUBLIC splCore cursiv::lsp)
  add_executable(spl_lsp spl/lsp/main.cpp)
  target_link_libraries(spl_lsp PRIVATE splLsp)
endif()

if(TARGET splLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spl/bench/main.cpp")
  cursiv_add_benchmark(spl
      SOURCES spl/bench/main.cpp)
endif()

cursiv_package_language(spl)
