# Generate the parser and its language module.
cursiv_generate_language(ID bdf
    GRAMMAR    bdf/core/BdfGrammar.hpp
    NAMESPACE  bdf::Bdf
    EXTENSIONS .bdf
    OUT        bdf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bdf/core/*.cpp)
add_library(bdfCore ${_core})
target_link_libraries(bdfCore PUBLIC bdfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bdf/cli/*.cpp)
if(_cli)
  add_executable(bdf_cli ${_cli})
  target_link_libraries(bdf_cli
                        PRIVATE bdfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bdf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bdf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bdf/lsp/main.cpp)
  add_library(bdfLsp ${_lsp})
  target_link_libraries(bdfLsp PUBLIC bdfCore cursiv::lsp)
  add_executable(bdf_lsp bdf/lsp/main.cpp)
  target_link_libraries(bdf_lsp PRIVATE bdfLsp)
endif()

if(TARGET bdfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bdf/bench/main.cpp")
  cursiv_add_benchmark(bdf
      SOURCES bdf/bench/main.cpp)
endif()

cursiv_package_language(bdf)
