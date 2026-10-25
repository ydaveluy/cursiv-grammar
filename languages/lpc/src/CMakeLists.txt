# Generate the parser and its language module.
cursiv_generate_language(ID lpc
    GRAMMAR    lpc/core/LpcGrammar.hpp
    NAMESPACE  lpc::Lpc
    EXTENSIONS .lpc
    OUT        lpc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lpc/core/*.cpp)
add_library(lpcCore ${_core})
target_link_libraries(lpcCore PUBLIC lpcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lpc/cli/*.cpp)
if(_cli)
  add_executable(lpc_cli ${_cli})
  target_link_libraries(lpc_cli
                        PRIVATE lpcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lpc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lpc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lpc/lsp/main.cpp)
  add_library(lpcLsp ${_lsp})
  target_link_libraries(lpcLsp PUBLIC lpcCore cursiv::lsp)
  add_executable(lpc_lsp lpc/lsp/main.cpp)
  target_link_libraries(lpc_lsp PRIVATE lpcLsp)
endif()

if(TARGET lpcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lpc/bench/main.cpp")
  cursiv_add_benchmark(lpc
      SOURCES lpc/bench/main.cpp)
endif()

cursiv_package_language(lpc)
