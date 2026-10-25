# Generate the parser and its language module.
cursiv_generate_language(ID oncrpc
    GRAMMAR    oncrpc/core/OncrpcGrammar.hpp
    NAMESPACE  oncrpc::Oncrpc
    EXTENSIONS .x
    OUT        oncrpc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/core/*.cpp)
add_library(oncrpcCore ${_core})
target_link_libraries(oncrpcCore PUBLIC oncrpcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/cli/*.cpp)
if(_cli)
  add_executable(oncrpc_cli ${_cli})
  target_link_libraries(oncrpc_cli
                        PRIVATE oncrpcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/lsp/main.cpp)
  add_library(oncrpcLsp ${_lsp})
  target_link_libraries(oncrpcLsp PUBLIC oncrpcCore cursiv::lsp)
  add_executable(oncrpc_lsp oncrpc/lsp/main.cpp)
  target_link_libraries(oncrpc_lsp PRIVATE oncrpcLsp)
endif()

if(TARGET oncrpcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/oncrpc/bench/main.cpp")
  cursiv_add_benchmark(oncrpc
      SOURCES oncrpc/bench/main.cpp)
endif()

cursiv_package_language(oncrpc)
