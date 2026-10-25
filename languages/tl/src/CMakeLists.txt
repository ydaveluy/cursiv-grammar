# Generate the parser and its language module.
cursiv_generate_language(ID tl
    GRAMMAR    tl/core/TlGrammar.hpp
    NAMESPACE  tl::Tl
    EXTENSIONS .tl
    OUT        tl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tl/core/*.cpp)
add_library(tlCore ${_core})
target_link_libraries(tlCore PUBLIC tlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tl/cli/*.cpp)
if(_cli)
  add_executable(tl_cli ${_cli})
  target_link_libraries(tl_cli
                        PRIVATE tlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tl/lsp/main.cpp)
  add_library(tlLsp ${_lsp})
  target_link_libraries(tlLsp PUBLIC tlCore cursiv::lsp)
  add_executable(tl_lsp tl/lsp/main.cpp)
  target_link_libraries(tl_lsp PRIVATE tlLsp)
endif()

if(TARGET tlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tl/bench/main.cpp")
  cursiv_add_benchmark(tl
      SOURCES tl/bench/main.cpp)
endif()

cursiv_package_language(tl)
