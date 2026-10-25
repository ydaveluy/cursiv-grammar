# Generate the parser and its language module.
cursiv_generate_language(ID rego
    GRAMMAR    rego/core/RegoGrammar.hpp
    NAMESPACE  rego::Rego
    EXTENSIONS .rego
    OUT        rego/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rego/core/*.cpp)
add_library(regoCore ${_core})
target_link_libraries(regoCore PUBLIC regoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rego/cli/*.cpp)
if(_cli)
  add_executable(rego_cli ${_cli})
  target_link_libraries(rego_cli
                        PRIVATE regoCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rego/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rego/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rego/lsp/main.cpp)
  add_library(regoLsp ${_lsp})
  target_link_libraries(regoLsp PUBLIC regoCore cursiv::lsp)
  add_executable(rego_lsp rego/lsp/main.cpp)
  target_link_libraries(rego_lsp PRIVATE regoLsp)
endif()

if(TARGET regoLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rego/bench/main.cpp")
  cursiv_add_benchmark(rego
      SOURCES rego/bench/main.cpp)
endif()

cursiv_package_language(rego)
