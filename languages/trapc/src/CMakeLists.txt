# Generate the parser and its language module.
cursiv_generate_language(ID trapc
    GRAMMAR    trapc/core/TrapcGrammar.hpp
    NAMESPACE  trapc::Trapc
    EXTENSIONS .tc
    OUT        trapc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trapc/core/*.cpp)
add_library(trapcCore ${_core})
target_link_libraries(trapcCore PUBLIC trapcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trapc/cli/*.cpp)
if(_cli)
  add_executable(trapc_cli ${_cli})
  target_link_libraries(trapc_cli
                        PRIVATE trapcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/trapc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/trapc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/trapc/lsp/main.cpp)
  add_library(trapcLsp ${_lsp})
  target_link_libraries(trapcLsp PUBLIC trapcCore cursiv::lsp)
  add_executable(trapc_lsp trapc/lsp/main.cpp)
  target_link_libraries(trapc_lsp PRIVATE trapcLsp)
endif()

if(TARGET trapcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/trapc/bench/main.cpp")
  cursiv_add_benchmark(trapc
      SOURCES trapc/bench/main.cpp)
endif()

cursiv_package_language(trapc)
