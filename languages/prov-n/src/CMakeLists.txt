# Generate the parser and its language module.
cursiv_generate_language(ID provn
    GRAMMAR    provn/core/ProvNGrammar.hpp
    NAMESPACE  provn::ProvN
    EXTENSIONS .provn
    OUT        provn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/provn/core/*.cpp)
add_library(provnCore ${_core})
target_link_libraries(provnCore PUBLIC provnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/provn/cli/*.cpp)
if(_cli)
  add_executable(provn_cli ${_cli})
  target_link_libraries(provn_cli
                        PRIVATE provnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/provn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/provn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/provn/lsp/main.cpp)
  add_library(provnLsp ${_lsp})
  target_link_libraries(provnLsp PUBLIC provnCore cursiv::lsp)
  add_executable(provn_lsp provn/lsp/main.cpp)
  target_link_libraries(provn_lsp PRIVATE provnLsp)
endif()

if(TARGET provnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/provn/bench/main.cpp")
  cursiv_add_benchmark(provn
      SOURCES provn/bench/main.cpp)
endif()

cursiv_package_language(provn)
