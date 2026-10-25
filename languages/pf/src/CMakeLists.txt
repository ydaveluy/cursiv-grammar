# Generate the parser and its language module.
cursiv_generate_language(ID pf
    GRAMMAR    pf/core/PfGrammar.hpp
    NAMESPACE  pf::Pf
    EXTENSIONS .pf
    OUT        pf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pf/core/*.cpp)
add_library(pfCore ${_core})
target_link_libraries(pfCore PUBLIC pfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pf/cli/*.cpp)
if(_cli)
  add_executable(pf_cli ${_cli})
  target_link_libraries(pf_cli
                        PRIVATE pfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pf/lsp/main.cpp)
  add_library(pfLsp ${_lsp})
  target_link_libraries(pfLsp PUBLIC pfCore cursiv::lsp)
  add_executable(pf_lsp pf/lsp/main.cpp)
  target_link_libraries(pf_lsp PRIVATE pfLsp)
endif()

if(TARGET pfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pf/bench/main.cpp")
  cursiv_add_benchmark(pf
      SOURCES pf/bench/main.cpp)
endif()

cursiv_package_language(pf)
