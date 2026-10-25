# Generate the parser and its language module.
cursiv_generate_language(ID bcpl
    GRAMMAR    bcpl/core/BcplGrammar.hpp
    NAMESPACE  bcpl::Bcpl
    EXTENSIONS .bcpl
    OUT        bcpl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/core/*.cpp)
add_library(bcplCore ${_core})
target_link_libraries(bcplCore PUBLIC bcplGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/cli/*.cpp)
if(_cli)
  add_executable(bcpl_cli ${_cli})
  target_link_libraries(bcpl_cli
                        PRIVATE bcplCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bcpl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/lsp/main.cpp)
  add_library(bcplLsp ${_lsp})
  target_link_libraries(bcplLsp PUBLIC bcplCore cursiv::lsp)
  add_executable(bcpl_lsp bcpl/lsp/main.cpp)
  target_link_libraries(bcpl_lsp PRIVATE bcplLsp)
endif()

if(TARGET bcplLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bcpl/bench/main.cpp")
  cursiv_add_benchmark(bcpl
      SOURCES bcpl/bench/main.cpp)
endif()

cursiv_package_language(bcpl)
