# Generate the parser and its language module.
cursiv_generate_language(ID pddl
    GRAMMAR    pddl/core/PddlGrammar.hpp
    NAMESPACE  pddl::Pddl
    EXTENSIONS .pddl
    OUT        pddl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pddl/core/*.cpp)
add_library(pddlCore ${_core})
target_link_libraries(pddlCore PUBLIC pddlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pddl/cli/*.cpp)
if(_cli)
  add_executable(pddl_cli ${_cli})
  target_link_libraries(pddl_cli
                        PRIVATE pddlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pddl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pddl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pddl/lsp/main.cpp)
  add_library(pddlLsp ${_lsp})
  target_link_libraries(pddlLsp PUBLIC pddlCore cursiv::lsp)
  add_executable(pddl_lsp pddl/lsp/main.cpp)
  target_link_libraries(pddl_lsp PRIVATE pddlLsp)
endif()

if(TARGET pddlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pddl/bench/main.cpp")
  cursiv_add_benchmark(pddl
      SOURCES pddl/bench/main.cpp)
endif()

cursiv_package_language(pddl)
