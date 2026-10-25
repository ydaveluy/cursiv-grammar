# Generate the parser and its language module.
cursiv_generate_language(ID dif
    GRAMMAR    dif/core/DifGrammar.hpp
    NAMESPACE  dif::Dif
    EXTENSIONS .dif
    OUT        dif/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dif/core/*.cpp)
add_library(difCore ${_core})
target_link_libraries(difCore PUBLIC difGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dif/cli/*.cpp)
if(_cli)
  add_executable(dif_cli ${_cli})
  target_link_libraries(dif_cli
                        PRIVATE difCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dif/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dif/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dif/lsp/main.cpp)
  add_library(difLsp ${_lsp})
  target_link_libraries(difLsp PUBLIC difCore cursiv::lsp)
  add_executable(dif_lsp dif/lsp/main.cpp)
  target_link_libraries(dif_lsp PRIVATE difLsp)
endif()

if(TARGET difLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dif/bench/main.cpp")
  cursiv_add_benchmark(dif
      SOURCES dif/bench/main.cpp)
endif()

cursiv_package_language(dif)
