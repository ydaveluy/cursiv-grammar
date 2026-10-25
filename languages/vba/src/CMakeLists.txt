# Generate the parser and its language module.
cursiv_generate_language(ID vba
    GRAMMAR    vba/core/VbaGrammar.hpp
    NAMESPACE  vba::Vba
    EXTENSIONS .bas .cls .frm
    OUT        vba/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vba/core/*.cpp)
add_library(vbaCore ${_core})
target_link_libraries(vbaCore PUBLIC vbaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vba/cli/*.cpp)
if(_cli)
  add_executable(vba_cli ${_cli})
  target_link_libraries(vba_cli
                        PRIVATE vbaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vba/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/vba/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vba/lsp/main.cpp)
  add_library(vbaLsp ${_lsp})
  target_link_libraries(vbaLsp PUBLIC vbaCore cursiv::lsp)
  add_executable(vba_lsp vba/lsp/main.cpp)
  target_link_libraries(vba_lsp PRIVATE vbaLsp)
endif()

if(TARGET vbaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vba/bench/main.cpp")
  cursiv_add_benchmark(vba
      SOURCES vba/bench/main.cpp)
endif()

cursiv_package_language(vba)
