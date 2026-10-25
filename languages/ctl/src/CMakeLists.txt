# Generate the parser and its language module.
cursiv_generate_language(ID ctl
    GRAMMAR    ctl/core/CtlGrammar.hpp
    NAMESPACE  ctl::Ctl
    EXTENSIONS .ctl
    OUT        ctl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ctl/core/*.cpp)
add_library(ctlCore ${_core})
target_link_libraries(ctlCore PUBLIC ctlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ctl/cli/*.cpp)
if(_cli)
  add_executable(ctl_cli ${_cli})
  target_link_libraries(ctl_cli
                        PRIVATE ctlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ctl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ctl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ctl/lsp/main.cpp)
  add_library(ctlLsp ${_lsp})
  target_link_libraries(ctlLsp PUBLIC ctlCore cursiv::lsp)
  add_executable(ctl_lsp ctl/lsp/main.cpp)
  target_link_libraries(ctl_lsp PRIVATE ctlLsp)
endif()

if(TARGET ctlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ctl/bench/main.cpp")
  cursiv_add_benchmark(ctl
      SOURCES ctl/bench/main.cpp)
endif()

cursiv_package_language(ctl)
