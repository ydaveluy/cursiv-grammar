# Generate the parser and its language module.
cursiv_generate_language(ID refal
    GRAMMAR    refal/core/RefalGrammar.hpp
    NAMESPACE  refal::Refal
    EXTENSIONS .ref
    OUT        refal/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/refal/core/*.cpp)
add_library(refalCore ${_core})
target_link_libraries(refalCore PUBLIC refalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/refal/cli/*.cpp)
if(_cli)
  add_executable(refal_cli ${_cli})
  target_link_libraries(refal_cli
                        PRIVATE refalCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/refal/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/refal/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/refal/lsp/main.cpp)
  add_library(refalLsp ${_lsp})
  target_link_libraries(refalLsp PUBLIC refalCore cursiv::lsp)
  add_executable(refal_lsp refal/lsp/main.cpp)
  target_link_libraries(refal_lsp PRIVATE refalLsp)
endif()

if(TARGET refalLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/refal/bench/main.cpp")
  cursiv_add_benchmark(refal
      SOURCES refal/bench/main.cpp)
endif()

cursiv_package_language(refal)
