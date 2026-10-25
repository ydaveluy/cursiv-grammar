# Generate the parser and its language module.
cursiv_generate_language(ID mps
    GRAMMAR    mps/core/MpsGrammar.hpp
    NAMESPACE  mps::Mps
    EXTENSIONS .mps
    OUT        mps/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mps/core/*.cpp)
add_library(mpsCore ${_core})
target_link_libraries(mpsCore PUBLIC mpsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mps/cli/*.cpp)
if(_cli)
  add_executable(mps_cli ${_cli})
  target_link_libraries(mps_cli
                        PRIVATE mpsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mps/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/mps/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mps/lsp/main.cpp)
  add_library(mpsLsp ${_lsp})
  target_link_libraries(mpsLsp PUBLIC mpsCore cursiv::lsp)
  add_executable(mps_lsp mps/lsp/main.cpp)
  target_link_libraries(mps_lsp PRIVATE mpsLsp)
endif()

if(TARGET mpsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mps/bench/main.cpp")
  cursiv_add_benchmark(mps
      SOURCES mps/bench/main.cpp)
endif()

cursiv_package_language(mps)
